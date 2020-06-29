/*
    Changhui Youn
    304207830
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define INPUT 'i'
#define OUTPUT 'o'
#define SEGFAULT 's'
#define CATCH 'c'

void sigsegv_handler(int sig){
    if(sig == SIGSEGV){
        fprintf(stderr, "Segmentation fault error!!\n");
        exit(4);
    }
}

int main (int argc, char* argv[]) {
    int opt;
    int ifd, ofd;
    int seg_flag;    // in order to catch segfault followed by --catch
    struct option options[] = {
        {"input", 1, NULL, INPUT},
        {"output", 1, NULL, OUTPUT},
        {"segfault", 0, NULL, SEGFAULT},
        {"catch", 0, NULL, CATCH},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
        switch (opt){
            case INPUT:
                ifd = open(optarg, O_RDONLY);
                if (ifd >= 0) {
                    close(0);
                    dup(ifd);
                    close(ifd);
                } else {
                    fprintf(stderr, "--input failed to open file %s : %s\n", optarg, strerror(errno));
                    exit(2);
                }
                break;
            case OUTPUT:
                ofd = creat(optarg, 0666);
                if (ofd >= 0) {
                    close(1);
                    dup(ofd);
                    close(ofd);
                } else {
                    fprintf(stderr, "--output failed to create file %s :%s\n", optarg, strerror(errno));
                    exit(3);
                }
                break;
            case SEGFAULT:
                seg_flag = 1;
                break;
            case CATCH:
                signal(SIGSEGV, sigsegv_handler);
                break;
            default:
                printf("Invalid argument\nYou may use any of following options : --input=filename --output=filename --segfault --catch\n");
                exit(1);
                break;
        }
    }
    
    if(seg_flag==1){
        char* ptr = NULL;
        *ptr = 0;
    }

    char buf;
    while((read(0, &buf, 1)) > 0){
        if(write(1, &buf, 1) < 0){
            fprintf(stderr, "Writing to file failed : %s\n", strerror(errno));
            exit(3);
        }
    }

    close(0);
    close(1);
    exit(0);
}
