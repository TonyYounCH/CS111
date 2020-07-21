/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <termios.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHELL 's'

struct termios saved_original;

int from_shell[2];
int to_shell[2];
int pid;

void restore(void) {
	tcsetattr(0, TCSANOW, &saved_original);
}

void terminal_setup(void) {
	struct termios tmp;
	tcgetattr(0, &saved_original);
	atexit(restore);

	tcgetattr(0, &tmp);
	tmp.c_iflag = ISTRIP, tmp.c_oflag = 0, tmp.c_lflag = 0;
	tcsetattr(0, TCSANOW, &tmp);
}

void signal_handler(int sig){
	if(sig == SIGPIPE){
		fprintf(stderr, "SIGPIPE received!");
		close(from_shell[0]);
		exit(0);
	}
}

int main(int argc, char* argv[]) {
	if(isatty(0) == 0){
		fprintf(stderr, "STDIN is not referring to a terminal");
		exit(1);
	}
	int opt;
	int shell_flag = 0;
	char* program;
	char rn[2] = {'\r', '\n'};

	struct option options[] = {
		{"shell", 1, NULL, SHELL},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case SHELL: 
				shell_flag = 1;
				program = optarg;
				break;
			default:
				fprintf(stderr, "Invalid argument(s)\nYou may only use --shell=program\n");
				exit(1);
				break;
		}
	}
	if(shell_flag) {
		if(pipe(to_shell) != 0) {
			fprintf(stderr, "Pipe to shell failed!\n");
			exit(1);
		}
		if(pipe(from_shell) != 0) {
			fprintf(stderr, "Pipe from shell failed!\n");
			exit(1);
		}

		terminal_setup();
		signal(SIGPIPE, signal_handler);

		pid = fork();
		if(pid == 0) {
			//Child Process
			close(to_shell[1]);
			close(from_shell[0]);
			close(0);
			dup(to_shell[0]);
			close(1);
			dup(from_shell[1]);
			close(2);
			dup(from_shell[1]);
			close(to_shell[0]);
			close(from_shell[1]);

			if(execlp(program, program, NULL) == -1) {
				fprintf(stderr, "Exec shell failed!\n");
				exit(1);
			}
		} else if (pid > 0) {
			//Parent Process
			close(to_shell[0]);
			close(from_shell[1]);

			struct pollfd pollfds[2];
			pollfds[0].fd = 0;
			pollfds[0].events = POLLIN | POLLHUP | POLLERR;
			pollfds[1].fd = from_shell[0];
			pollfds[1].events = POLLIN | POLLHUP | POLLERR;

			int end_loop = 0;

			while (!end_loop) {
				if((poll(pollfds, 2, -1)) > 0) {
					if(pollfds[0].revents == POLLIN) {
						char buffer[256];
						int res = read(0, &buffer, 256);
						if(res < 0) {
							fprintf(stderr, "Reading from input failed. Error: %d\n", errno);
							exit(1);
						}
						int i;
						for(i = 0; i < res; i++) {
							if(buffer[i] == '\r' || buffer[i] == '\n'){ 
								if((write(1, rn, sizeof(char)*2)) < 0) {
									fprintf(stderr, "Writing to STDOUT failed. Error: %d\n", errno);
									exit(1);
								}
								if((write(to_shell[1], &rn[1], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to SHELL failed. Error: %d\n", errno);
									exit(1);
								}
							} else if(buffer[i] == 0x04){
								printf("^D\n");
								close(to_shell[1]);
								end_loop = 1;
							} else if(buffer[i] == 0x03){
								printf("^C\n");
								kill(pid, SIGINT);
							} else { 
								if((write(1, &buffer[i], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to STDOUT failed. Error: %d\n", errno);
									exit(1);
								}
								if((write(to_shell[1], &buffer[i], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to SHELL failed. Error: %d\n", errno);
									exit(1);
								}
							}
						}
					// Handles EOF/Error from the stdout/stderr of the child process
					} else if(pollfds[0].revents & POLLHUP){
						// hup : fd is closed by other end
						close(from_shell[0]);
						end_loop = 1;
					} else if (pollfds[0].revents & POLLERR) {
						// err : err occured
						close(from_shell[0]);
						exit(1);
					}

					if(pollfds[1].revents == POLLIN) {
						char buffer[256];
						int res = read(from_shell[0], &buffer, 256);
						if(res < 0) {
							fprintf(stderr, "Reading from input failed. Error: %d\n", errno);
							exit(1);
						}
						int i;
						for(i = 0; i < res; i++) {
							if(buffer[i] == '\n'){ 
								if((write(1, rn, sizeof(char)*2)) < 0) {
									fprintf(stderr, "Writing to STDOUT failed. Error: %d\n", errno);
									exit(1);
								}
							} else { 
								if((write(1, &buffer[i], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to STDOUT failed. Error: %d\n", errno);
									exit(1);
								}
							}
						}
					} else if (pollfds[1].revents & (POLLERR | POLLHUP)) {
						close(from_shell[0]);
						end_loop = 1;
					} 
				} 
			}

			int status;
			waitpid(pid, &status, 0);
			fprintf(stderr, "\r\nSHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(status), WEXITSTATUS(status));
			exit(0);
		} else {
			fprintf(stderr, "Fork failed\n");
			exit(1);
		}
	}
	terminal_setup();
	char buf;
	while((read(0, &buf, 5)) > 0) {
		if(buf == 0x04){
			exit(0);
		} else if (buf == '\r' || buf == '\n'){
			write(1, rn, sizeof(char)*2);
		} else {
			write(1, &buf, sizeof(char));
		}
	}
	exit(0);

}
