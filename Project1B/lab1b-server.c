/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include "zlib.h"

#define PORT 'p'
#define SHELL 's'
#define COMP 'c'

struct termios saved_original;

int from_shell[2];
int to_shell[2];
int pid;
int socket_fd;
int comp_flag = 0;
z_stream to_client;
z_stream from_client;

void signal_handler(int sig){
	if(sig == SIGPIPE){
		fprintf(stderr, "SIGPIPE received!");
		close(from_shell[0]);
		exit(0);
	} else if(sig == SIGINT) {
		kill(pid, SIGINT);
	}
}

int server_connect(unsigned int port_num) {
	int sockfd, new_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	unsigned int sin_size;

	// sock
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "TCP/IP socket creation failed\n");
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port_num);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

	if(bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1){
		//some error handling
		fprintf(stderr, "Bind to socket failed\n");
		exit(1);
	}

	if(listen(sockfd, 5) == -1) {
		//some error handling
		fprintf(stderr, "Listening socket failed\n");
		exit(1);
	}

	while(1) {
		sin_size = sizeof(struct sockaddr_in);
		if((new_fd = accept(sockfd, (struct sockaddr*) &their_addr, &sin_size)) == -1){
			//some error handling
			fprintf(stderr, "Accept inputs from socket failed\n");
			exit(1);
			continue;
		} else {
			close(sockfd);
			break;
		}
	}
	return new_fd;
}


void init_compress_stream (z_stream * stream) {
	stream->zalloc = Z_NULL, stream-> zfree = Z_NULL, stream-> opaque = Z_NULL;
	if (deflateInit(stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
		fprintf(stderr, "Server : deflateInit failed\n");
		exit(1);
	}
}

void init_decompress_stream (z_stream * stream) {
	stream->zalloc = Z_NULL, stream-> zfree = Z_NULL, stream-> opaque = Z_NULL;
	if (inflateInit(stream) != Z_OK) {
		fprintf(stderr, "Server : inflateInit failed\n");
		exit(1);
	}
}

void compress_stream (z_stream * stream, void * orig_buf, int orig_len, void * out_buf, int out_len) {

	stream->next_in = orig_buf, stream->avail_in = orig_len;
	stream->next_out = out_buf, stream->avail_out = out_len;
	do {
		deflate(stream, Z_SYNC_FLUSH);
	} while (stream->avail_in > 0);
}

void decompress_stream (z_stream * stream, void * orig_buf, int orig_len, void * out_buf, int out_len) {
	stream->next_in = orig_buf, stream->avail_in = orig_len;
	stream->next_out = out_buf, stream->avail_out = out_len;
	do {
		inflate(stream, Z_SYNC_FLUSH);
	} while (stream->avail_in > 0);
}

void at_exit_signal(){
	close(socket_fd);
}

void comp_end(){
	deflateEnd(&to_client);
	inflateEnd(&from_client);
}

void harvest()
{	
	if(comp_flag) {
		comp_end();
	}
	close(socket_fd);
	int status;
	waitpid(pid, &status, 0);
	fprintf(stderr, "\r\nSHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(status), WEXITSTATUS(status));
	exit(0);
}

int main(int argc, char* argv[]) {
	struct option options[] = {
		{"port", 1, NULL, PORT},
		{"shell", 1, NULL, SHELL},
		{"compress", 0, NULL, COMP},
		{0, 0, 0, 0}
	};

	int port_no = 0;
	int mandatory = 0;
	char rn[2] = {'\r', '\n'};
	char* program = "/bin/bash";

	int opt;
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case PORT: 
				port_no = atoi(optarg);
				mandatory = 1;
				break;
			case SHELL: 
				program = optarg;
				break;
			case COMP: 
				comp_flag = 1;
				init_compress_stream(&to_client);
				init_decompress_stream(&from_client);

				break;
			default:
				fprintf(stderr, "Invalid argument(s)\nYou may use --port=port# (mandatory), --shell=program, or --compress only.\n");
				exit(1);
				break;
		}
	}
	if(!mandatory) {
		fprintf(stderr, "--port=port# option is mandatory\n");
		exit(1);
	}

	socket_fd = server_connect(port_no);
	//set the terminal to the non-canonical, no echo mode
	//register sigpipe handler create pipe, for process, stdin/out redirection
	if(pipe(to_shell) != 0) {
		fprintf(stderr, "Pipe to shell failed!\n");
		exit(1);
	}
	if(pipe(from_shell) != 0) {
		fprintf(stderr, "Pipe from shell failed!\n");
		exit(1);
	}

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
		pollfds[0].fd = socket_fd;
		pollfds[0].events = POLLIN | POLLHUP | POLLERR;
		pollfds[1].fd = from_shell[0];
		pollfds[1].events = POLLIN | POLLHUP | POLLERR;

		int end_loop = 0;

		atexit(harvest);
		
		while (!end_loop) {
			if((poll(pollfds, 2, -1)) > 0) {
				if(pollfds[0].revents == POLLIN) {
					char buffer[256];
					int res = read(socket_fd, &buffer, 256);
					if(res < 0) {
						fprintf(stderr, "Reading from socket failed. Error: %d\n", errno);
						exit(1);
					}

					if (comp_flag) {
						char out_buf[1024];
						int out_len = 1024;
						decompress_stream(&from_client, buffer, res, out_buf, out_len);

						int i;
						for (i = 0; (unsigned int) i < out_len - from_client.avail_out; i++) {
							if(out_buf[i] == '\r' || out_buf[i] == '\n'){ 
								if((write(to_shell[1], &rn[1], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to SHELL failed. Error: %d\n", errno);
									exit(1);
								}
							} else if(out_buf[i] == 0x04){
								close(to_shell[1]);
								end_loop = 1;
							} else if(out_buf[i] == 0x03){
								kill(pid, SIGINT);
							} else { 
								if((write(to_shell[1], &out_buf[i], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to SHELL failed. Error: %d\n", errno);
									exit(1);
								}
							}
						}
					} else {
						int i;
						for(i = 0; i < res; i++) {
							if(buffer[i] == '\r' || buffer[i] == '\n'){ 
								if((write(to_shell[1], &rn[1], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to SHELL failed. Error: %d\n", errno);
									exit(1);
								}
							} else if(buffer[i] == 0x04){
								close(to_shell[1]);
								end_loop = 1;
							} else if(buffer[i] == 0x03){
								kill(pid, SIGINT);
							} else { 
								if((write(to_shell[1], &buffer[i], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to SHELL failed. Error: %d\n", errno);
									exit(1);
								}
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

					int i, j;
					int count = 0;

					for(i = 0, j = 0; i < res; i++) {
						if(buffer[i] == 0x04){
							end_loop = 1;
						} else if(buffer[i] == '\n'){ 
							if(comp_flag) {
								char out_buf[256];
								int out_len = 256;

								compress_stream(&to_client, (buffer+j), count, out_buf, out_len);
								write(socket_fd, out_buf, out_len - to_client.avail_out);

								char out_buf2[256];
								int out_len2 = 256;

								compress_stream(&to_client, rn, 2, out_buf2, out_len2);
								write(socket_fd, out_buf2, out_len2 - to_client.avail_out);
								j += count + 1;
								count = 0;
							} else {
								if((write(socket_fd, rn, sizeof(char)*2)) < 0) {
									fprintf(stderr, "Writing to socket failed. Error: %d\n", errno);
									exit(1);
								}
							}
						} else {
							if(comp_flag) {
								count++;
							} else {
								if((write(socket_fd, &buffer[i], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to socket failed. Error: %d\n", errno);
									exit(1);
								}
							}
						}
					}

					if(comp_flag) {
						char out_buf[256];
						int out_len = 256;

						compress_stream(&to_client, (buffer+j), count, out_buf, out_len);
						write(socket_fd, out_buf, out_len - to_client.avail_out);
					}
				} else if (pollfds[1].revents & (POLLERR | POLLHUP)) {
					close(from_shell[0]);
					end_loop = 1;
				} 
			} 
		}

	} else {
		fprintf(stderr, "Fork failed\n");
		exit(1);
	}
	exit(0);
	return 0;
}