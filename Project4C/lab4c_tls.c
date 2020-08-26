/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/
#ifdef DUMMY
/*
	Below is dummy code that only executes when -DDUMMY compile
	It declares few functions that are in mraa
*/
#include <stdlib.h>
struct _aio {
	int pin;
};

typedef struct _aio* mraa_aio_context;

int mraa_aio_read(mraa_aio_context c) {
	c->pin = 1;
	return 650;
}

void mraa_aio_close(mraa_aio_context c) {
	c->pin = 1;
	return;
}

mraa_aio_context mraa_aio_init(int num) {
	mraa_aio_context c = malloc(sizeof(struct _aio));
	c->pin = num;
	return c;
}

void mraa_deinit() {
	return;
}

#else
#include <stdlib.h>
#include <mraa.h>
#include <mraa/aio.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include "fcntl.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PERIOD 'p'
#define SCALE 's'
#define LOG 'l'
#define ID 'i'
#define HOST 'h'

//** REMEMBER TO CHANGE EXIT CODES
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k
int period = 1;
char flag = 'F';
struct pollfd polls[1];
int log_fd;
int log_flag = 0;
int stop = 0;
int port;
int sock_fd = 0;
struct sockaddr_in server_address;
struct hostent *server;
char* hostname = NULL;
char* id;
mraa_aio_context temp;
SSL* ssl;

void print_errors(char* error){
    if(strcmp(error, "temp") == 0){
        fprintf(stderr, "Failed to initialize temperature sensor\n");
        mraa_deinit();
        exit(1);
    }
    if(strcmp(error, "usage") == 0) {
        fprintf(stderr, "Incorrect argument: correct usage is ./lab4a --period=# [--scale=tempOpt] [--log=filename]\n");
        exit(1);
    }
    if(strcmp(error, "file") == 0) {
        fprintf(stderr, "Failed to create file\n");
        exit(2);
    }
    if(strcmp(error, "poll") == 0){
        fprintf(stderr, "Failed to poll\n");
        exit(2);
    }
    if(strcmp(error, "read") == 0){
        fprintf(stderr, "Failed to read\n");
        exit(2);
    }
    if(strcmp(error, "period") == 0){
        fprintf(stderr, "Period of 0 is not legal\n");
        exit(1);
    }
    if(strcmp(error, "socket") == 0){
        fprintf(stderr, "Error creating socket\n");
        exit(2);
    }
    if(strcmp(error, "connection") == 0){
        fprintf(stderr, "Error establishing connection to server\n");
        exit(2);
    }
    if(strcmp(error, "id_length") == 0){
        fprintf(stderr, "ID length is not 9. Inavlid ID\n");
        exit(1);
    }
    if(strcmp(error, "host") == 0){
        fprintf(stderr, "failed to get host name\n");
        exit(1);
    }
    if(strcmp(error, "ssl") == 0){
        fprintf(stderr, "failed to initialize SSL\n");
        exit(1);
    }
    if(strcmp(error, "ctx") == 0){
        fprintf(stderr, "failed to intialize SSL context\n");
        exit(2);
    }
    if(strcmp(error, "ssl conenction") == 0){
        fprintf(stderr, "failed to establish ssl connection\n");
        exit(2);
    }
    if(strcmp(error, "ssl write") == 0){
        fprintf(stderr, "failed to do write over SSL\n");
        exit(2);
    }
    if(strcmp(error, "set_fd") == 0){
        fprintf(stderr, "failed to associate file descriptor\n");
        exit(2);
    }
}

void write_message(char* message) {
    if(SSL_write(ssl, message, strlen(message))< 0){
        print_errors("ssl write");
    }
}

void create_report(float temperature) {
	char buf[256];
	struct timespec ts;
	struct tm * tm;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&(ts.tv_sec));
	sprintf(buf, "%.2d:%.2d:%.2d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temperature);
	write_message(buf);
	if(log_flag && !stop) {
		dprintf(log_fd, "%.2d:%.2d:%.2d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temperature);
	}
}//help with time from https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm


// Initializes the sensors
void initialize_the_sensors() {
	temp = mraa_aio_init(1);
	if (temp == NULL) {
		fprintf(stderr, "Failed to init aio\n");
		exit(1);
	}
}

// convert whatever output from the seonsor to desired scale
float convert_temper_reading(int reading) {
	float R = 1023.0/((float) reading) - 1.0;
	float R0 = 100000.0;
	float B = 4275;
	R = R0*R;
	//C is the temperature in Celcious
	float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	//F is the temperature in Fahrenheit
	float F = (C * 9)/5 + 32;
	if(flag == 'C')
		return C;
	else
		return F;
}

void do_when_interrupted() {
	char buf[256];
	struct timespec ts;
	struct tm * tm;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&(ts.tv_sec));
	sprintf(buf, "%.2d:%.2d:%.2d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
	write_message(buf);
	if(log_flag) {
		dprintf(log_fd, "%.2d:%.2d:%.2d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	SSL_shutdown(ssl);
	SSL_free(ssl);
	exit(0);
}

// This function processes stdin
void process_stdin(char *input) {
    if(strcmp(input, "SCALE=F") == 0){
        flag = 'F';
        if(log_flag)
            dprintf(log_fd, "SCALE=F\n");
    } else if(strcmp(input, "SCALE=C") == 0){
        flag = 'C';
        if(log_flag)
            dprintf(log_fd, "SCALE=C\n");
    } else if(strncmp(input, "PERIOD=", sizeof(char)*7) == 0){
        period = (int)atoi(input+7);
        if(log_flag)
            dprintf(log_fd, "PERIOD=%d\n", period);
    } else if(strcmp(input, "STOP") == 0){
        stop = 1;
        if(log_flag)
            dprintf(log_fd, "STOP\n");
    } else if(strcmp(input, "START") == 0){
        stop = 0;
        if(log_flag)
            dprintf(log_fd, "START\n");
    } else if((strncmp(input, "LOG", sizeof(char)*3) == 0)){
        if(log_flag){
            dprintf(log_fd, "%s\n", input);
        }
    } else if(strcmp(input, "OFF") == 0){
        if(log_flag)
            dprintf(log_fd, "OFF\n");
        do_when_interrupted();
    } else {
        fprintf(stderr, "Command cannot be recognized\n");
        exit(1);
    }
}


void deinit_sensors(){
    mraa_aio_close(temp);
    close(log_fd);
}


void setup_connection() {
	if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Failed to create socket in client\n");
	}
	if ((server = gethostbyname(hostname)) == NULL){
		fprintf(stderr, "No host\n");
	}
	memset((void*)&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	memcpy((char *) &server_address.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	server_address.sin_port = htons(port);
	if(connect(sock_fd, (struct sockaddr*) &server_address, sizeof(server_address))< 0){
		fprintf(stderr, "Failed to connect to socket\n");
		exit(1);
	}
}

void setupPollandTime(){
    char commandBuff[128];
    char copyBuff[128];
    memset(commandBuff, 0, 128);
    memset(copyBuff, 0, 128);
    int copyIndex = 0;
    polls[0].fd = sock_fd;
    polls[0].events = POLLIN | POLLERR | POLLHUP;
    for(;;){
        int value = mraa_aio_read(temp);
        float tempValue = convert_temper_reading(value);
        if(!stop){
            create_report(tempValue);
        }
        time_t begin, end;
        time(&begin);
        time(&end); //start begin and end at the same time and keep running loop until period is reached
        while(difftime(end, begin) < period){
            int ret = poll(polls, 1, 0);
            if(ret < 0){
                print_errors("poll");
            }
            if(polls[0].revents && POLLIN){
                int num = SSL_read(ssl, commandBuff, 128);
                if(num < 0){
                    print_errors("read");
                }
                int i;
                for(i = 0; i < num && copyIndex < 128; i++){
                    if(commandBuff[i] =='\n'){
                        process_stdin((char*)&copyBuff);
                        copyIndex = 0;
                        memset(copyBuff, 0, 128); //clear
                    }
                    else {
                        copyBuff[copyIndex] = commandBuff[i];
                        copyIndex++;
                    }
                }
                
            }
            time(&end);
        }
    }
}//help with time https://www.tutorialspoint.com/c_standard_library/c_function_time.htm

void setup_ssl() {
	OpenSSL_add_all_algorithms();
	SSL_library_init();
	SSL_CTX *ssl_ctx = SSL_CTX_new(TLSv1_client_method());
	if(ssl_ctx == NULL){
		fprintf(stderr, "Failed to get ssl context\n");
		exit(2);
	}
	if((ssl = SSL_new(ssl_ctx)) == NULL){
		fprintf(stderr, "Failed to setup ssl\n");
		exit(2);
	}
	if(SSL_set_fd(ssl, sock_fd)<0) {
		fprintf(stderr, "Failed to associate sock_fd -> ssl\n");
		exit(2);
	}
	if(SSL_connect(ssl) != 1){
		fprintf(stderr, "Failed to establish ssl Connection\n");
		exit(2);
	}
}

void send_id() {
    char buffer[64];
    setup_ssl();
    sprintf(buffer, "ID=%s\n", id);
    write_message(buffer);
    dprintf(log_fd, "ID=%s\n", id);
}


int main(int argc, char** argv){
    int opt = 0;
    static struct option options [] = {
        {"period", 1, 0, 'p'},
        {"scale", 1, 0, 's'},
        {"log", 1, 0, 'l'},
        {"id", 1, 0, 'i'},
        {"host", 1, 0, 'h'},
        {0, 0, 0, 0}
    };
    
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case PERIOD: 
				// get checking period
				period = atoi(optarg);
				break;
			case SCALE:
				// get iteration #
				if (optarg[0] == 'F' || optarg[0] == 'C') {
					flag = optarg[0];
				} else {
					fprintf(stderr, "Invalid argument(s)\n--scale option only accepts [C, F]\n");
					exit(1);
				}
				break;
			case LOG:
				// log file
				log_flag = 1;
				char* filename = optarg;
				if ((log_fd = creat(filename, 0666)) < 0) {
					fprintf(stderr, "--log=filename failed to create/write to file\n");
					exit(1);
				}
				break;
			case ID:
				id = optarg;
				break;
			case HOST:
				hostname = optarg;
				break;
			default:
				fprintf(stderr, "Invalid argument(s)\nYou may use --period=#, --scale=[C,F], --log=filepath, --host=hostname, --id=#\n");
				exit(1);
				break;
		}
	}

	port = atoi(argv[optind]);
	if (port <= 0) {
		fprintf(stderr, "Invalid port number\n");
		exit(1);
	}

	if (strlen(hostname) == 0) {
		fprintf(stderr, "--host is mandatory\n");
		exit(1);
	}
	if (log_fd == -1) {
		fprintf(stderr, "--log is mandatory\n");
		exit(1);
	}
	if (strlen(id) != 9) {
		fprintf(stderr, "ID must be 9 digit\n");
		exit(1);
	} 

    // close(STDIN_FILENO); //close input
    setup_connection();
    send_id();
    initialize_the_sensors();

    struct pollfd pollfd;
    pollfd.fd = sock_fd;
    pollfd.events = POLLIN;

    char buffer[256];
    char full_command[256];
    memset(buffer, 0, 256);
    memset(full_command, 0, 256);
    int index = 0;
    while (1) {
        // if it is time to report temperature && !stop
        // read from temperature sensor, convert and report
        report_temp();

         // use poll syscalls, no or very short< 50ms timeout interval
        if(poll(&pollfd, 1, 0) < 0){
            fprintf(stderr, "Failed to read from poll\n");
        }
        if(pollfd.revents && POLLIN){
            int res = SSL_read(ssl, buffer, 256);
            if(res < 0){
                fprintf(stderr, "Failed to read from STDIN_FILENO\n");
            }
            int i;
            for(i = 0; i < res && index < 256; i++){
                if(buffer[i] =='\n'){
                    process_stdin((char*)&full_command);
                    index = 0;
                    memset(full_command, 0, 256);
                } else {
                    full_command[index] = buffer[i];
                    index++;
                }
            }
            
        } 
    }


    deinit_sensors();
    exit(0);
}
//button is no longer used as a method of shutdown

//IF IT DOENS'T WORK USE THE STRING THING FOR SSL

