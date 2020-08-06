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
#define LIST 'l'

#define SEC_TO_NSEC 1000000000L

SortedList_t* listheads;
SortedListElement_t* pool;

int opt_yield = 0;
char opt_sync = 0;
long threads = 1;
long iterations = 1;
long num_elements = 0;
long num_list = 1;

pthread_mutex_t* mutexes;
int* spin_lock;

char test[16] = "list-";
char* str_yield = NULL;

// This function sets test string based on str_yield and opt_sync
void set_test(){
	if(str_yield == NULL){
		str_yield = "none";
	}
	strcat(test, str_yield);
	if(opt_sync == 'm'){
		strcat(test, "-m");
	}
	else if(opt_sync == 's'){
		strcat(test, "-s");
	}
	else {
		strcat(test, "-none");
	}
}

int hashkey(const char* key) {
	// a simple hash of the key modulo the nuber of lists
	return key[0] % num_list;
}

void * thread_worker(void* arg) {
	int i;
	long wait_time = 0;
	unsigned long num = *((unsigned long*) arg);
	struct timespec begin, end;
	for(i = num; i < num_elements; i += threads){
		// create hash key that corresponds to element key
		int hash = hashkey(pool[i].key);
		// SortedList_insert is critical section and it is
		// wrapped with mutex lock or spin_lock based on opt_sync
		if(opt_sync == 'm') {
			if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			} 
			pthread_mutex_lock(&mutexes[hash]);
			if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			}
		} else if(opt_sync == 's') {
			if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			} 
			while(__sync_lock_test_and_set(&spin_lock[hash], 1));
			if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			}
		} 
		SortedList_insert(&listheads[hash], &pool[i]);
		if(opt_sync == 'm') {
			wait_time += SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
			pthread_mutex_unlock(&mutexes[hash]);
		} else if(opt_sync == 's') {
			wait_time += SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
			__sync_lock_release(&spin_lock[hash]);
		} 
	}

	int length = 0;
	// SortedList_length is critical section and it is
	// wrapped with mutex lock or spin_lock based on opt_sync
	for (i = 0; i < num_list; i++) {
		if(opt_sync == 'm') {
			if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			} 
			pthread_mutex_lock(&mutexes[i]);
			if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			}
		} else if(opt_sync == 's') {
			if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			} 
			while(__sync_lock_test_and_set(&spin_lock[i], 1));
			if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			}
		} 
		length = SortedList_length(&listheads[i]);
		if (length < 0) {
			fprintf(stderr, "Failed to read length\n");
			exit(2);
		}
		if(opt_sync == 'm') {
			wait_time += SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
			pthread_mutex_unlock(&mutexes[i]);
		} else if(opt_sync == 's') {
			wait_time += SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
			__sync_lock_release(&spin_lock[i]);
		} 
	}

	SortedListElement_t *element;

	for (i = num; i < num_elements; i += threads) {
		// SortedList_lookup and SortedList_delete are critical section
		// and they are wrapped with mutex lock or spin_lock based on opt_sync
		int hash = hashkey(pool[i].key);
		if(opt_sync == 'm') {
			if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			} 
			pthread_mutex_lock(&mutexes[hash]);
			if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			}
		} else if(opt_sync == 's') {
			if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			} 
			while(__sync_lock_test_and_set(&spin_lock[hash], 1));
			if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
				fprintf(stderr, "Gettime error\n");
				exit(1);
			}
		} 
		element = SortedList_lookup(&listheads[hash], pool[i].key);
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
			wait_time += SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
			pthread_mutex_unlock(&mutexes[hash]);
		} else if(opt_sync == 's') {
			wait_time += SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
			__sync_lock_release(&spin_lock[hash]);
		} 
	}
	return (void *) wait_time;
}

void signal_handler(int sigNum){
	// handles segmentation fault
	if(sigNum == SIGSEGV){
		fprintf(stderr, "Segmentation fault caught!\n");
		exit(2);
	}
}


char* rand_key(){
	// generates randome string
	// ref : https://stackoverflow.com/questions/15767691/whats-the-c-library-function-to-generate-random-string
	char* random_key = (char*) malloc(sizeof(char)*10);
	int i;
	for (i=0; i<9; i++){
		random_key[i] = '0' + rand()%72;
	}
	random_key[9] = '\0';
	return random_key;
}


// This function initialize pool with num_elements
void init_elem(int num_elements) {
	int i;
	// seed rand()
	srand(time(NULL));
	for (i = 0; i < num_elements; i++) {
		pool[i].key = rand_key();
	}
}

void print_csv(long threads, long iterations, long num_list, long num_operations, long run_time, long avg_per_op, long lock_time){
	fprintf(stdout, "%s,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n", test, threads, iterations, num_list, num_operations, run_time, avg_per_op, lock_time);
}

int main(int argc, char *argv[]) {
	int opt = 0;
	threads = 1;
	iterations = 1;
	opt_yield = 0;
	opt_sync = 0;
	num_list = 1;

	struct option options[] = {
		{"threads", 1, NULL, THREAD},
		{"iterations", 1, NULL, ITER},
		{"yield", 1, NULL, YIELD},
		{"sync", 1, NULL, SYNC},
		{"lists", 1, NULL, LIST},
		{0, 0, 0, 0}
	};

	ssize_t i;
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case THREAD: 
				// get threads #
				threads = atoi(optarg);
				break;
			case ITER:
				// get iteration #
				iterations = atoi(optarg);
				break;
			case YIELD:
				// get opt_yield and given string
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
				str_yield = (char*) malloc(sizeof(char)*5);
				str_yield = optarg;
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
			case LIST:
				num_list = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Invalid argument(s)\nYou may use --threads=#, --iterations=#, --yield=[idl], --sync=[m,s], --lists=#\n");
				exit(1);
				break;
		}
	}
	signal(SIGSEGV, signal_handler);

	num_elements = threads * iterations;

	// listheads is currently empty and needs initialization
	listheads = (SortedList_t*) malloc(sizeof(SortedList_t) * num_list);
	for(i = 0; i < (ssize_t) num_list; i++){
		listheads[i].prev = &listheads[i];
		listheads[i].next = &listheads[i];
		listheads[i].key = NULL;
	}
	// set up the locks
	if(opt_sync == 'm') {
		mutexes = malloc(sizeof(pthread_mutex_t) * num_list);
		for (i = 0; i < num_list; i++) {
			if (pthread_mutex_init(&mutexes[i], NULL) != 0) {
				fprintf(stderr, "Fail to initialize mutex");
				exit(1);
			}
		}
	} else if (opt_sync == 's') {
		spin_lock = malloc(sizeof(int) * num_list);
		for (i = 0; i < num_list; i++) {
			spin_lock[i] = 0;
		}
	}
	pool = (SortedListElement_t*) malloc(sizeof(SortedListElement_t)*num_elements);
	init_elem(num_elements);

	struct timespec begin, end;
	if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
		fprintf(stderr, "Gettime error\n");
		exit(1);
	} 

	pthread_t *thread = malloc((sizeof(pthread_t) * threads));
	int thread_id[threads];

	for(i = 0; i < threads; i++) {
		thread_id[i] = i;
		if(pthread_create(&thread[i], NULL, thread_worker, &thread_id[i]) < 0){
			fprintf(stderr, "Thread creation failed\n");
			exit(1);
		}
	}
	long total_wait_time = 0;
	void** wait_time = (void *) malloc(sizeof(void**));
	for(i = 0; i < threads; i++) {
		if(pthread_join(thread[i], wait_time) < 0){
			fprintf(stderr, "Thread join failed\n");
			exit(1);
		}
		// adds all wait time that thread_worker function returns
		total_wait_time += (long) *wait_time;
	}

	// stop the timer
	if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
		fprintf(stderr, "Gettime error\n");
		exit(1);
	}

	if(SortedList_length(listheads) != 0){
		fprintf(stderr, "Length of list is not 0\n");
		exit(2);
	}

	long num_operations = threads * iterations * 3;
	long run_time = SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
	long avg_per_op = run_time / num_operations;
	long lock_time = total_wait_time / num_operations;

	// print corresponding data
	set_test();
	print_csv(threads, iterations, num_list, num_operations, run_time, avg_per_op, lock_time);
	if(opt_sync == 'm'){
		for(i = 0; i < (ssize_t) num_list; i++)
			pthread_mutex_destroy(&mutexes[i]);
		free(mutexes);
	} else if(opt_sync == 's')
		free(spin_lock);

	free(thread);
	free(pool);
	free(wait_time);
	exit(0);


}




