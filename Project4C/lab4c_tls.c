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
#include <signal.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>

#define PERIOD 'p'
#define SCALE 's'
#define LOG 'l'
#define ID 'i'
#define HOST 'h'

int period = 1;
char scale = 'F';
int stop = 0;
time_t begin = 0;
time_t end = 0;
int log_flag = 0;
int log_fd = -1;
int port;

mraa_aio_context temp;

struct hostent *server;
char* hostname = "";
char* id;
struct sockaddr_in server_address;
int sock_fd = 0;
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

void print_to_server(char *str) {
	if(SSL_write(ssl, str, strlen(str) + 1) < 0){
		fprintf(stderr, "Failed to write to ssl\n");
	}
}
// This shuts down and prints SHUTDOWN message to output
void do_when_interrupted() {
	char buf[256];
	struct timespec ts;
	struct tm * tm;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&(ts.tv_sec));
	sprintf(buf, "%.2d:%.2d:%.2d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
	print_to_server(buf);
	if(log_flag) {
		dprintf(log_fd, "%.2d:%.2d:%.2d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	SSL_shutdown(ssl);
	SSL_free(ssl);
	exit(0);
}

// This prints out executing time and read temperature 
void curr_temp_report(float temperature){
	char buf[256];
	struct timespec ts;
	struct tm * tm;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&(ts.tv_sec));
	sprintf(buf, "%.2d:%.2d:%.2d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temperature);
	print_to_server(buf);
	if(log_flag && !stop) {
		dprintf(log_fd, "%.2d:%.2d:%.2d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temperature);
	}
}

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
	if(scale == 'C')
		return C;
	else
		return F;
}

void report_temp() {
	// if it is time to report temperature && !stop
	// read from temperature sensor, convert and report
	time(&end);
	if(difftime(end, begin) >= period && !stop) {
		time(&begin);
		int reading = mraa_aio_read(temp);
		float temperature = convert_temper_reading(reading);
		curr_temp_report(temperature);
	}
}

// This function processes stdin
void process_stdin(char *input) {
	if(strcmp(input, "SCALE=F") == 0){
		scale = 'F';
		if(log_flag)
			dprintf(log_fd, "SCALE=F\n");
	} else if(strcmp(input, "SCALE=C") == 0){
		scale = 'C';
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

void handle_scale(char flag) {
    switch(flag){
        case 'C':
        case 'c':
            scale = 'C';
            if(log_flag && stop == 0){
                dprintf(log_fd, "SCALE=C\n");
            }
            break;
        case 'F':
        case 'f':
            scale = 'F';
            if(log_flag && stop == 0){
                dprintf(log_fd, "SCALE=F\n");
            }
            break;
        default:
            fprintf(stderr, "Incorrect scale option");
            break;
    }
}

void handle_shutdown() {
    time_t raw;
    struct tm* currTime;
    time(&raw);
    currTime = localtime(&raw);
    char buff[256];
    sprintf(buff, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
    print_to_server(buff);
    if(log_flag) {
        dprintf(log_fd, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
    }
    exit(0);
}

void handle_off() {
    if(log_flag){
        dprintf(log_fd, "OFF\n");
    }
    handle_shutdown();
}

void handle_period(int newPeriod) {
    period = newPeriod;
    if(log_flag && stop == 0){
        dprintf(log_fd, "PERIOD=%d\n", period);
    }
}

void handle_stop() {
    stop = 1;
    if(log_flag){
        dprintf(log_fd, "STOP\n");
    }
}

void handle_start() {
    stop = 0;
    if(log_flag){
        dprintf(log_fd, "START\n");
    }
}

void handle_log(const char* input) {
    if(log_flag){
        dprintf(log_fd, "%s\n", input);
    }
}


void handle_input(const char* input) {
    if(strcmp(input, "OFF") == 0){
        handle_off();
    }
    else if(strcmp(input, "START") == 0){
        handle_start();
    }
    else if(strcmp(input, "STOP") == 0){
        handle_stop();
    }
    else if(strcmp(input, "SCALE=F") == 0){
        handle_scale('F');
    }
    else if(strcmp(input, "SCALE=C") == 0){
        handle_scale('C');
    }
    else if(strncmp(input, "PERIOD=", sizeof(char)*7) == 0){
        int newPeriod = (int)atoi(input+7);
        handle_period(newPeriod);
    }
    else if((strncmp(input, "LOG", sizeof(char)*3) == 0)){
        handle_log(input);
    }
    else {
        fprintf(stderr, "Command not recognized\n");
        exit(1);
    }
}

void setupPollandTime(){
    char commandBuff[128];
    char copyBuff[128];
    memset(commandBuff, 0, 128);
    memset(copyBuff, 0, 128);
    int copyIndex = 0;
    struct pollfd polls[1];
    polls[0].fd = sock_fd;
    polls[0].events = POLLIN | POLLERR | POLLHUP;
    for(;;){
        int value = mraa_aio_read(temp);
        double tempValue = convert_temper_reading(value);
        if(!stop){
            curr_temp_report(tempValue);
        }
        time_t begin, end;
        time(&begin);
        time(&end); //start begin and end at the same time and keep running loop until period is reached
        while(difftime(end, begin) < period){
            poll(polls, 1, 0);
            // if(ret < 0){
            //     print_errors("poll");
            // }
            if(polls[0].revents && POLLIN){
                int num = SSL_read(ssl, commandBuff, 128);
                // if(num < 0){
                //     print_errors("read");
                // }
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

void setupConnection() {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0){
        print_errors("socket");
    }
    server = gethostbyname(hostname);
    if (server == NULL){
        print_errors("host");
    } //check if hostname is retrieved
    memset((char*)&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(portNum);
    if(connect(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address))< 0){
        print_errors("connection");
    }
}


void initSSL() {
    OpenSSL_add_all_algorithms();
    if(SSL_library_init() < 0){
        print_errors("ssl");
    }
    SSL_CTX *ssl_ctx = SSL_CTX_new(TLSv1_client_method());
    if(ssl_ctx == NULL){
        print_errors("ctx");
    }
    ssl = SSL_new(ssl_ctx);
    if(SSL_set_fd(ssl, sock_fd)<0) {
        print_errors("set_fd");
    }
    if(SSL_connect(ssl) != 1){
        print_errors("ssl connection");
    }
}//documentation from https://www.openssl.org/docs/manmaster/man7/ssl.html

int main(int argc, char* argv[]) {
	int opt = 0;
	struct option options[] = {
		{"period", 1, NULL, PERIOD},
		{"scale", 1, NULL, SCALE},
		{"log", 1, NULL, LOG},
		{"id", 1, NULL, ID},
		{"host", 1, NULL, HOST},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "p:sl", options, NULL)) != -1) {
        switch(opt){
            case 'p':
                period = (int)atoi(optarg);
                if(period == 0){
                    print_errors("period");
                }
                break;
            case 's':
                switch(*optarg){
                    case 'C':
                    case 'c':
                        scale = 'C';
                        break;
                    case 'F':
                    case 'f':
                        scale = 'F';
                        break;
                    default:
                        print_errors("usage");
                        break;
                }
                break;
            case 'l':
                log_flag = 1;
                char* logFile = optarg;
                log_fd = creat(logFile, 0666);
                if(log_fd < 0){
                    print_errors("file");
                }
                break;
            case 'i':
                if(strlen(optarg) != 9){
                    print_errors("id_length");
                }
                id = optarg;
                break;
            case 'h':
                hostname = optarg;
                break;
            default:
                print_errors("usage");
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

	// setup_connection();
	setupConnection();
	// setup_ssl();
	initSSL();

	char buf[32];
	sprintf(buf, "ID=%s", id);
	print_to_server(buf);
	dprintf(log_fd, "ID=%s\n", id);

	initialize_the_sensors();
	setupPollandTime();
	// struct pollfd pollfd;
	// pollfd.fd = sock_fd;
	// pollfd.events = POLLIN;

	// char buffer[256];
	// char full_command[256];
	// memset(buffer, 0, 256);
	// memset(full_command, 0, 256);
	// int index = 0;
	// while (1) {
	// 	// if it is time to report temperature && !stop
	// 	// read from temperature sensor, convert and report
	// 	report_temp();

	// 	 // use poll syscalls, no or very short< 50ms timeout interval
	// 	if(poll(&pollfd, 1, 0) < 0){
	// 		fprintf(stderr, "Failed to read from poll\n");
	// 	}
	// 	if(pollfd.revents && POLLIN){
	// 		int res = SSL_read(ssl, buffer, 256);
	// 		if(res < 0){
	// 			fprintf(stderr, "Failed to read from STDIN_FILENO\n");
	// 		}
	// 		int i;
	// 		for(i = 0; i < res && index < 256; i++){
	// 			if(buffer[i] =='\n'){
	// 				process_stdin((char*)&full_command);
	// 				index = 0;
	// 				memset(full_command, 0, 256);
	// 			} else {
	// 				full_command[index] = buffer[i];
	// 				index++;
	// 			}
	// 		}
			
	// 	} 
	// }

	mraa_aio_close(temp);
	close(log_fd);

	return 0;



}