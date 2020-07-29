/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/

#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sched.h>

#define THREAD 't'
#define ITER 'i'
#define YIELD 'y'
#define SYNC 's'

#define SEC_TO_NSEC 1000000000L

long long counter;
int opt_yield;
char opt_sync;
pthread_mutex_t mutex;

long threads;
long iterations;
int spin_lock = 0;

// This is critical section that adds given value
void add(long long *pointer, long long value) {
	long long sum = *pointer + value;
	if(opt_yield)
		sched_yield();
	*pointer = sum;
}

// This function handles critical section based on opt_sync
void task(unsigned long iter, int val){
	unsigned long i = 0;
	if(opt_sync == 'm'){
		for(i=0; i < iter; i++) {
			// mutex lock
			pthread_mutex_lock(&mutex);
			add(&counter, val);
			pthread_mutex_unlock(&mutex);
		}
	} else if (opt_sync == 's') {
		for(i=0; i < iter; i++) {
			// spin lock
			while(__sync_lock_test_and_set(&spin_lock, 1));
			add(&counter, val);
			__sync_lock_release(&spin_lock);
		}
	} else if (opt_sync == 'c') {
		// compare and swap
		long long old;
		do {
			old = counter;
			if (opt_yield) {
				sched_yield();
			}
		} while (__sync_val_compare_and_swap(&counter, old, old + val) != old);
	} else {
		for(i=0; i < iter; i++) {
			add(&counter, val);
		}
	}

}

void* thread_worker(void* arg) {
	// add 1 and subtract 1 so that it evens out
	unsigned long iter = *((unsigned long*) arg);
	task(iter, 1);
	task(iter, -1);
	return NULL;
}

void print_csv(char* test, long threads, long iterations, long num_operations, long run_time, long avg_per_op, long long counter){
	fprintf(stdout, "%s,%ld,%ld,%ld,%ld,%ld,%lld\n", test, threads, iterations, num_operations, run_time, avg_per_op, counter);
}

int main(int argc, char *argv[]) {
	// threads, iteration = process_arg(argc, argv);
	int opt = 0;
	counter = 0;
	threads = 1;
	iterations = 1;
	opt_yield = 0;
	opt_sync = 0;

	struct option options[] = {
		{"threads", 1, NULL, THREAD},
		{"iterations", 1, NULL, ITER},
		{"yield", 0, NULL, YIELD},
		{"sync", 1, NULL, SYNC},
		{0, 0, 0, 0}
	};

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
				// Does it yield?
				opt_yield = 1;
				break;
			case SYNC:
				if(strlen(optarg) != 1) {
					fprintf(stderr, "Invalid argument : --sync only takes m, s, or c as an argument\n");
					exit(1);
				}
				opt_sync = *optarg;
				if(opt_sync != 'm' && opt_sync != 's' && opt_sync != 'c'){
					fprintf(stderr, "Invalid argument : --sync only takes m, s, or c as an argument\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Invalid argument(s)\nYou may use --threads=#, --iterations=#, --sync=[m,s,c]\n");
				exit(1);
				break;
		}
	}

	pthread_t *thread = malloc((sizeof(pthread_t) * threads));

	struct timespec begin, end;
	// start the timer
	int i;
	if(clock_gettime(CLOCK_MONOTONIC, &begin) < 0){
		fprintf(stderr, "Gettime error\n");
		exit(1);
	} 
	for(i = 0; i < threads; i++) {
		// create threads with worker function
		if(pthread_create(&thread[i], NULL, thread_worker, &iterations) < 0){
			fprintf(stderr, "Thread creation failed\n");
			exit(1);
		}
	}
	for(i = 0; i < threads; i++) {
		// join threads
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
	
	long num_operations = threads * iterations * 2;
	long run_time = SEC_TO_NSEC * (end.tv_sec - begin.tv_sec) + end.tv_nsec - begin.tv_nsec;
	long avg_per_op = run_time/num_operations;

	// print corresponding data
	if(opt_yield == 1 && opt_sync == 'm')
		print_csv("add-yield-m", threads, iterations, num_operations, run_time, avg_per_op, counter);
	else if(opt_yield == 1 && opt_sync == 's')
		print_csv("add-yield-s", threads, iterations, num_operations, run_time, avg_per_op, counter);
	else if(opt_yield == 1 && opt_sync == 'c')
		print_csv("add-yield-c", threads, iterations, num_operations, run_time, avg_per_op, counter);
	else if (opt_yield == 1 && opt_sync == 0)
		print_csv("add-yield-none", threads, iterations, num_operations, run_time, avg_per_op, counter);
	else if (opt_yield == 0 && opt_sync == 'm')
		print_csv("add-m", threads, iterations, num_operations, run_time, avg_per_op, counter);
	else if (opt_yield == 0 && opt_sync == 's')
		print_csv("add-s", threads, iterations, num_operations, run_time, avg_per_op, counter);
	else if (opt_yield == 0 && opt_sync == 'c')
		print_csv("add-c", threads, iterations, num_operations, run_time, avg_per_op, counter);
	else
		print_csv("add-none", threads, iterations, num_operations, run_time, avg_per_op, counter);

	if(opt_sync == 'm')
		pthread_mutex_destroy(&mutex);

	free(thread);
	exit(0);
}