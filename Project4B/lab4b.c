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
#define MRAA_GPIO_IN 0
#define MRAA_GPIO_EDGE_RISING 1
struct _aio {
	int pin;
};
struct _gpio {
	int pin;
};
typedef struct _aio* mraa_aio_context;
typedef struct _gpio* mraa_gpio_context;;
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

mraa_gpio_context mraa_gpio_init(int num) {
	mraa_gpio_context c = malloc(sizeof(struct _gpio));
	c->pin = num;
	return c;
}

void mraa_gpio_dir(mraa_gpio_context c, int i) {
	c->pin = i;
	return;
}

void mraa_gpio_isr(mraa_gpio_context c, int i, void* fun, void* k){
	if(c || i || fun || k)
		return;
	return;
}

void mraa_gpio_close(mraa_gpio_context c) {
	c->pin = 1;
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

#define PERIOD 'p'
#define SCALE 's'
#define LOG 'l'

int period = 1;
char scale = 'F';
int stop = 0;
time_t begin = 0;
time_t end = 0;
int log_flag = 0;
int logfd = 0;

mraa_aio_context temp;
mraa_gpio_context button;

// This shuts down and prints SHUTDOWN message to output
void do_when_interrupted() {
	struct timespec ts;
	struct tm * tm;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&(ts.tv_sec));
	fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
	if(log_flag) {
		dprintf(logfd, "%.2d:%.2d:%.2d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	exit(0);
}

// This prints out executing time and read temperature 
void curr_temp_report(float temperature){
	struct timespec ts;
	struct tm * tm;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&(ts.tv_sec));
	fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temperature);
	if(log_flag && !stop) {
		dprintf(logfd, "%.2d:%.2d:%.2d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temperature);
	}
}

// Initializes the sensors
void initialize_the_sensors() {
	temp = mraa_aio_init(1);
	if (temp == NULL) {
		fprintf(stderr, "Failed to init aio\n");
		exit(1);
	}

	button = mraa_gpio_init(60);
	if (button == NULL) {
		fprintf(stderr, "Failed to init button\n");
		exit(1);
	}

	mraa_gpio_dir(button, MRAA_GPIO_IN);
	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &do_when_interrupted, NULL);

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
		// if(DUM)
		// 	reading = 650;
		float temperature = convert_temper_reading(reading);
		curr_temp_report(temperature);
	}
}

// This function processes stdin
void process_stdin(char *input) {
	if(strcmp(input, "SCALE=F") == 0){
		scale = 'F';
		if(log_flag)
			dprintf(logfd, "SCALE=F\n");
	} else if(strcmp(input, "SCALE=C") == 0){
		scale = 'C';
		if(log_flag)
			dprintf(logfd, "SCALE=C\n");
	} else if(strncmp(input, "PERIOD=", sizeof(char)*7) == 0){
		period = (int)atoi(input+7);
		if(log_flag)
			dprintf(logfd, "PERIOD=%d\n", period);
	} else if(strcmp(input, "STOP") == 0){
		stop = 1;
		if(log_flag)
			dprintf(logfd, "STOP\n");
	} else if(strcmp(input, "START") == 0){
		stop = 0;
		if(log_flag)
			dprintf(logfd, "START\n");
	} else if((strncmp(input, "LOG", sizeof(char)*3) == 0)){
		if(log_flag){
			dprintf(logfd, "%s\n", input);
		}
	} else if(strcmp(input, "OFF") == 0){
		if(log_flag)
			dprintf(logfd, "OFF\n");
		do_when_interrupted();
	} else {
		fprintf(stdout, "Command cannot be recognized\n");
		exit(1);
	}
}


int main(int argc, char* argv[]) {
	int opt = 0;
	struct option options[] = {
		{"period", 1, NULL, PERIOD},
		{"scale", 1, NULL, SCALE},
		{"log", 1, NULL, LOG},
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
					scale = optarg[0];
				} else {
					fprintf(stderr, "Invalid argument(s)\n--scale option only accepts [C, F]\n");
					exit(1);
				}
				break;
			case LOG:
				// log file
				log_flag = 1;
				char* filename = optarg;
				if ((logfd = creat(filename, 0666)) < 0) {
					fprintf(stderr, "--log=filename failed to create/write to file\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Invalid argument(s)\nYou may use --period=#, --scale=[C,F], --log=filepath\n");
				exit(1);
				break;
		}
	}

	initialize_the_sensors();

	struct pollfd pollfd;
	pollfd.fd = 0;
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
			int res = read(0, buffer, 256);
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

		// if (push button is pressed) 
		// 	log and exit. 
	}

	mraa_aio_close(temp);
	mraa_gpio_close(button);

	return 0;



}