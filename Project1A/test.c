int ctrl_c = 0;
			int i;

			while (!ctrl_c) {
				if ((poll(pollfds, 2, -1)) > 0) {
					if (pollfds[0].revents & POLLIN) {
						char buffer[256];
						int res = read(0, &buffer, 256);
						if(res < 0) {
							fprintf(stderr, "Reading from input failed. Error: %d\n", errno);
							exit(1);
						}
						for(i = 0; i < res; i++) {
							if (buffer[i] == '\r' || buffer[i] == '\n'){ 
								if((write(1, rn, sizeof(char)*2)) < 0) {
									fprintf(stderr, "Writing to STDOUT failed. Error: %d\n", errno);
									exit(1);
								}
								if((write(to_shell[1], &rn[1], sizeof(char))) < 0) {
									fprintf(stderr, "Writing to SHELL failed. Error: %d\n", errno);
									exit(1);
								}
							} else if (buffer[i] == 0x04){
								ctrl_c = 1;
							} else if (buffer[i] == 0x03){
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
						ctrl_c = 1;
					} else if (pollfds[0].revents & POLLERR) {
						// err : err occured
						fprintf(stderr, "Poll error in STDIN\n");
						exit(1);

					}

					if (pollfds[1].revents == POLLIN) {
						char input[256];
						int num = read(from_shell[0], &input, 256); 
						int count = 0;
						int j;
						for (i = 0, j = 0; i < num; i++) {
							if (input[i] == 0x04) { //EOF from shell
								ctrl_c = 1;
							} else if (input[i] == '\n') {
								write(STDOUT_FILENO, (input + j), count);
								write(STDOUT_FILENO, rn, sizeof(char)*2);
								j += count + 1;
								count = 0;
								continue;
							}
							count++;
						}
						write(STDOUT_FILENO, (input+j), count);		

					} else if (pollfds[1].revents & POLLERR || pollfds[1].revents & POLLHUP) { //polling error
						ctrl_c = 1;
					} 
				} 
			}