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
#include <assert.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "zlib.h"

#define PORT 'p'
#define LOG 'l'
#define COMP 'c'

struct termios saved_original;
int socket_fd;
int lfd;

// This function restores terminal state to its normal state
void restore(void) {
	close(socket_fd);
	close(lfd);
	tcsetattr(0, TCSANOW, &saved_original);
}

// This function sets up the terminal to non-canonical mode 
// and saves the normal state
void terminal_setup(void) {
	struct termios tmp;
	tcgetattr(0, &saved_original);
	atexit(restore);

	tcgetattr(0, &tmp);
	tmp.c_iflag = ISTRIP, tmp.c_oflag = 0, tmp.c_lflag = 0;
	tcsetattr(0, TCSANOW, &tmp);
}

// Create TCP/IP socket and connects to server with the socket with given
// hostname and port number
int client_connect(char* hostname, unsigned int port) {
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent* server;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		//error
		fprintf(stderr, "TCP/IP socket creation failed\n");
		exit(1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	server = gethostbyname(hostname);
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1){
		//error
		fprintf(stderr, "Connection to socket failed\n");
		exit(1);
	}
	return sockfd;
}

// This inits compress stream
void init_compress_stream (z_stream * stream) {
	stream->zalloc = Z_NULL, stream-> zfree = Z_NULL, stream-> opaque = Z_NULL;
	if (deflateInit(stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
		fprintf(stderr, "Client : deflateInit failed\n");
		exit(1);
	}
}

// This inits decompress stream
void init_decompress_stream (z_stream * stream) {
	stream->zalloc = Z_NULL, stream-> zfree = Z_NULL, stream-> opaque = Z_NULL;
	if (inflateInit(stream) != Z_OK) {
		fprintf(stderr, "Client : inflateInit failed\n");
		exit(1);
	}
}


// deflate until all orinal buffer has no more data in it
void compress_stream (z_stream * stream, void * orig_buf, int orig_len, void * out_buf, int out_len) {
	stream->next_in = orig_buf, stream->avail_in = orig_len;
	stream->next_out = out_buf, stream->avail_out = out_len;
	do {
		deflate(stream, Z_SYNC_FLUSH);
	} while (stream->avail_in > 0);
}

// inflate until all orinal buffer has no more data in it
void decompress_stream (z_stream * stream, void * orig_buf, int orig_len, void * out_buf, int out_len) {
	stream->next_in = orig_buf, stream->avail_in = orig_len;
	stream->next_out = out_buf, stream->avail_out = out_len;
	do {
		inflate(stream, Z_SYNC_FLUSH);
	} while (stream->avail_in > 0);
}

int main(int argc, char* argv[]) {
	
	struct option options[] = {
		{"port", 1, NULL, PORT},
		{"log", 1, NULL, LOG},
		{"compress", 0, NULL, COMP},
		{0, 0, 0, 0}
	};
	int port_no = 0;
	int mandatory = 0;
	int log_flag = 0;
	int comp_flag = 0;
	char rn[2] = {'\r', '\n'};
	char* hostname = "localhost";

	z_stream to_server;
	z_stream from_server;

	int opt;
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case PORT: 
				// set port# to given number
				port_no = atoi(optarg);
				mandatory = 1;
				break;
			case LOG: 
				// set log flag
				log_flag = 1;
				if ((lfd = creat(optarg, 0666)) < 0) {
					fprintf(stderr, "--log=filename failed to create/write to file\n");
					exit(1);
				}
				break;
			case COMP: 
				// set compression flag and initialize two streams
				comp_flag = 1;
				init_compress_stream(&to_server);
				init_decompress_stream(&from_server);
				break;
			default:
				// invalid argument is given
				fprintf(stderr, "Invalid argument(s)\nYou may use --port=port# (mandatory), --log=filename, or --compress only.\n");
				exit(1);
				break;
		}
	}
	if(!mandatory) {
		fprintf(stderr, "--port=port# option is mandatory\n");
		exit(1);
	}

	//set the terminal to the non-canonical, no echo mode
	terminal_setup();

	// create connection to TCP/IP socket
	socket_fd = client_connect(hostname, port_no);


	struct pollfd pollfds[2];
	pollfds[0].fd = 0;
	pollfds[0].events = POLLIN | POLLHUP | POLLERR;
	pollfds[1].fd = socket_fd;
	pollfds[1].events = POLLIN | POLLHUP | POLLERR;

	while(1){
		if((poll(pollfds, 2, -1)) > 0) {
			if(pollfds[0].revents == POLLIN) {
				// read from stdin, process special chars, send to stdout and socket_fd
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
					}  else {
						if((write(1, &buffer[i], sizeof(char))) < 0) {
							fprintf(stderr, "Writing to STDOUT failed. Error: %d\n", errno);
							exit(1);
						}
					}
				}
				if(comp_flag) {
					// Compression
					char out_buf[256];
					int out_len = 256;

					// Compress original buffer to out_buf and write the compressed
					// data into socket_fd
					compress_stream(&to_server, buffer, res, out_buf, out_len);
					write(socket_fd, out_buf, out_len - to_server.avail_out);

					if (log_flag) {
						dprintf(lfd, "SENT %d bytes: ", out_len - to_server.avail_out);
						write(lfd, (char*) out_buf, out_len - to_server.avail_out);
						write(lfd, &rn[1], sizeof(char));
					}

				} else {
					// No need to compress
					if((write(socket_fd, buffer, res)) < 0) {
						fprintf(stderr, "Writing to SOCKET failed. Error: %d\n", errno);
						exit(1);
					}
					if (log_flag) {
						// Log
						dprintf(lfd, "SENT %d bytes: ", res);
						write(lfd, buffer, res);
						write(lfd, &rn[1], sizeof(char));
					}
				}
			} else if(pollfds[0].revents & POLLHUP){
				exit(0);
			} else if (pollfds[0].revents & POLLERR) {
				// err : err occurred
				fprintf(stderr, "Error occurred with STDIN poll\n");
				exit(1);
			}

			if(pollfds[1].revents == POLLIN) {
				// read from socket_fd, process special chars, send to stdout
				char buffer[256];
				int res = read(socket_fd, &buffer, 256);
				if(res < 0) {
					fprintf(stderr, "Reading from input failed. Error: %d\n", errno);
					exit(1);
				}
				if(res == 0) {
					exit(0);
				}

				if(comp_flag) {
					char out_buf[1024];
					int out_len = 1024;

					// Deompress original buffer to out_buf and write the compressed
					// data into STDOUT
					decompress_stream(&from_server, buffer, res, out_buf, out_len);
					write(1, out_buf, out_len - from_server.avail_out);

				} else {
					if((write(1, buffer, res)) < 0) {
						fprintf(stderr, "Writing to STDOUT from socket failed. Error: %d\n", errno);
						exit(1);
					}
				}

				if(log_flag){
					// Log
					dprintf(lfd, "RECEIVED %d bytes: ", res);
					write(lfd, (char*) buffer, res);
					write(lfd, &rn[1], sizeof(char));

				}
			} else if (pollfds[1].revents & (POLLERR | POLLHUP)) {
				exit(0);
			} 
		}

	}

	if (comp_flag) {
		inflateEnd(&from_server);
		deflateEnd(&to_server);
	}
}