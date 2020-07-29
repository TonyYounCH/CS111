/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "SortedList.h"


#define THREAD 't'
#define ITER 'i'
#define YIELD 'y'
#define SYNC 's'

#define SEC_TO_NSEC 1000000000L

SortedList_t head;
SortedListElement_t* pool;

int opt_yield;
int opt_yield;
char opt_sync;
long threads;
long iterations;
long num_elements;

pthread_mutex_t mutex;
int spin_lock = 0;


void * thread_worker(void* arg) {
	int i;
	unsigned long num = *((unsigned long*) arg);
	for(i = num; i < num_elements; i += threads){
		if(opt_sync == 'm') {
			pthread_mutex_lock(&mutex);
		} else if(opt_sync == 's') {
			while(__sync_lock_test_and_set(&spin_lock, 1));
		} 
		SortedList_insert(&head, &pool[i]);
		if(opt_sync == 'm') {
			pthread_mutex_unlock(&mutex);
		} else if(opt_sync == 's') {
			__sync_lock_release(&spin_lock);
		} 
	}

	int length = 0;
	if(opt_sync == 'm') {
		pthread_mutex_lock(&mutex);
	} else if(opt_sync == 's') {
		while(__sync_lock_test_and_set(&spin_lock, 1));
	} 
	length = SortedList_length(&head);
	if (length < 0) {
		fprintf(stderr, "Failed to read length\n");
		exit(2);
	}
	if(opt_sync == 'm') {
		pthread_mutex_unlock(&mutex);
	} else if(opt_sync == 's') {
		__sync_lock_release(&spin_lock);
	} 

	SortedListElement_t *element;

	for (i = num; i < num_elements; i += threads) {
		if(opt_sync == 'm') {
			pthread_mutex_lock(&mutex);
		} else if(opt_sync == 's') {
			while(__sync_lock_test_and_set(&spin_lock, 1));
		} 
    	element = SortedList_lookup(&head, pool[i].key);
    	if (element == NULL) {
    		fprintf(stderr, "Failure to look up element\n");
    		exit(2);
    	}

		int n = SortedList_delete(element);
    	if (n != 0) {
    		fprintf(stderr, "Failure to delete element\n");
    		exit(2);
    	}
		if(opt_sync == 'm') {
			pthread_mutex_unlock(&mutex);
		} else if(opt_sync == 's') {
			__sync_lock_release(&spin_lock);
		} 
	}
	return NULL;
}

void list_remove(SortedListElement_t* element) {
	SortedListElement_t* n = element->next; //say n = 0x1234	
	SortedListElement_t* p = element->prev;	
	n->prev = p;
	p->next = n;
	element->next = NULL;
	element->prev = NULL;
}

void signal_handler(int sigNum){
	if(sigNum == SIGSEGV){
		fprintf(stderr, "Segmentation fault caught!\n");
		exit(2);
	}
}//signal handler for SIGSEGV


char* rand_key(){
	char* random_key = (char*) malloc(sizeof(char)*10);
	int i;
	for (i=0; i<9; i++){
		random_key[i] = (char) rand()%26 + 'a';
	}
	random_key[9] = '\0';
	return random_key;
}//https://stackoverflow.com/questions/19724346/generate-random-characters-in-c generating random characters in C


void init_elem(int num_elements) {
	int i;
	for (i = 0; i < num_elements; i++) {
		pool[i].key = rand_key();
	}
}

void print_csv(long threads, long iterations, long val, long num_operations, long run_time, long avg_per_op){
	fprintf(stdout, ",%ld,%ld,%ld,%ld,%ld,%ld\n", threads, iterations, val, num_operations, run_time, avg_per_op);
}

int main(int argc, char *argv[]) {
	int opt = 0;
	threads = 1;
	iterations = 1;
	opt_yield = 0;
	opt_sync = 0;

	struct option options[] = {
		{"threads", 1, NULL, THREAD},
		{"iterations", 1, NULL, ITER},
		{"yield", 1, NULL, YIELD},
		{"sync", 1, NULL, SYNC},
		{0, 0, 0, 0}
	};

	ssize_t i;
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case THREAD: 
				threads = atoi(optarg);
				break;
			case ITER:
				iterations = atoi(optarg);
				break;
			case YIELD:
				for (i = 0; i < (ssize_t) strlen(optarg); i++) {
					if (optarg[i] == 'i') {
						opt_yield |= INSERT_YIELD;
					} else if (optarg[i] == 'd') {
						opt_yield |= DELETE_YIELD;
					} else if (optarg[i] == 'l') {
						opt_yield |= LOOKUP_YIELD;
					} else {
						fprintf(stderr, "Invalid argument : --yield only takes i, d, or l as an argument\n");
						exit(1);
					}
				}
				break;
			case SYNC:
				if(strlen(optarg) != 1) {
					fprintf(stderr, "Invalid argument : --sync only takes m or s as an argument\n");
					exit(1);
				}
				opt_sync = *optarg;
				if(opt_sync != 'm' && opt_sync != 's'){
					fprintf(stderr, "Invalid argument : --sync only takes m or s as an argument\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Invalid argument(s)\nYou may use --threads=#, --iterations=#, --sync=[m,s]\n");
				exit(1);
				break;
		}
	}
	signal(SIGSEGV, signal_handler);

	num_elements = threads * iterations;
	pool = malloc(sizeof(SortedListElement_t) * num_elements);
	init_elem(num_elements);

	struct timespec begin, end;
	if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
		fprintf(stderr, "Gettime error\n");
		exit(1);
	} 

	pthread_t *thread = malloc((sizeof(pthread_t) * threads));
	for(i = 0; i < threads; i++) {
		if(pthread_create(&thread[i], NULL, thread_worker, &threads) < 0){
			fprintf(stderr, "Thread creation failed\n");
			exit(1);
		}
	}
	for(i = 0; i < threads; i++) {
		if(pthread_join(thread[i], NULL) < 0){
			fprintf(stderr, "Thread creation failed\n");
			exit(1);
		}
	}

	// stop the timer
	if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
		fprintf(stderr, "Gettime error\n");
		exit(1);
	}

    if(SortedList_length(pool) != 0){
        fprintf(stderr, "Length of list is not 0\n");
        exit(2);
    }

	long num_operations = threads * iterations * 3;
	long run_time = SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
	long avg_per_op = run_time/num_operations;

	// print corresponding data
  	fprintf(stdout, "list");
	switch(opt_yield) {
		case 0:
			fprintf(stdout, "-none");
			break;
		case 1:
			fprintf(stdout, "-i");
			break;
		case 2:
			fprintf(stdout, "-d");
			break;
		case 3:
			fprintf(stdout, "-id");
			break;
		case 4:
			fprintf(stdout, "-l");
			break;
		case 5:
			fprintf(stdout, "-il");
			break;
		case 6:
			fprintf(stdout, "-dl");
			break;
		case 7:
			fprintf(stdout, "-idl");
			break;
		default:
			break;
	}
	  
	switch(opt_sync) {
		case 0:
			fprintf(stdout, "-none");
			break;
		case 's':
			fprintf(stdout, "-s");
			break;
		case 'm':
			fprintf(stdout, "-m");
			break;
		default:
			break;
	}
	print_csv(threads, iterations, 1, num_operations, run_time, avg_per_op);
	if(opt_sync == 'm')
		pthread_mutex_destroy(&mutex);

	free(thread);
	free(pool);
	exit(0);


}




