/******************************************************************************
* Author: Andrew Johnson
* Date: 4/26/2020
* Pledge: I pledge my honor that I have abided by the Stevens Honor System.
* Description: Performs sieve of erathoneses and finds 
* 			   amount of primes with at least two '3's.
******************************************************************************/

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

int total_count = 0;
pthread_mutex_t lock;


typedef struct arg_struct {
    int start;
    int end;
} thread_args;

void display_usage() {
	printf("Usage: ./mtsieve -s <starting value> -e <ending value> -t <num threads>\n");
}

int count_three(int num){
	int count = 0;
	while(num){
		if(num%10 == 3){
			count++;
		}
		num/=10;
	}
	if(count >= 2){
		return 1;
	}
	return 0;
}

void simp_sieve(int limit, int *primes){
	bool mark[limit + 1];
	memset(mark, false, limit);
	int index = 0;
	for(int i = 2; i < limit; ++i){
		if(mark[i] == false){
			primes[index] = i;
			index++;
			for(int j = i; j<= limit; j+= i){
				mark[j] = true;
			}
		}
	}
	
}

void *seg_sieve(void *ptr){
	thread_args *point = (thread_args *)ptr;
	int start = point->start;
	int end = point->end;
	int limit = floor(sqrt(end));
	int low_primes[limit];
	memset(low_primes, 0, sizeof(low_primes));
	simp_sieve(limit, low_primes);

	int n = end - start + 1;
	bool *mark = (bool *)malloc(sizeof(bool) * (n+1));
	memset(mark, false, sizeof(*mark));
	
	for(int i = 0; i < sizeof(low_primes); i++){
		if(low_primes[i] == 0){
			break;
		}
		int loLim = floor(start/low_primes[i]) * low_primes[i];
		if(loLim < start){
			loLim += low_primes[i];
		}
		if(loLim == low_primes[i]){
			loLim += low_primes[i];
		}
		for(int j = loLim; j <= end; j+=low_primes[i]){
			mark[j - start] = true;
		}
	}
	int three = 0;
	for(int i = start; i <= end; i++){
		if(!mark[i - start]){
			three += count_three(i);
		}
	}
	if(pthread_mutex_lock(&lock) != 0){
        fprintf(stderr, "Warning: Cannot lock mutex. %s.\n", strerror(pthread_mutex_lock(&lock)));
    }
	total_count += three;
	if(pthread_mutex_unlock(&lock) != 0){
        fprintf(stderr, "Warning: Cannot unlock mutex. %s.\n", strerror(pthread_mutex_unlock(&lock)));
    }
	free(mark);
	pthread_exit(NULL);
}


bool is_integer(char* input) {
	int start = 0, len = strlen(input);

	if (len >= 1 && input[0] == '-') {
		if (len < 2) {
			return false;
		}
		start = 1;
	}
	for (int i = start; i < len; ++i) {
		if (!isdigit(input[i])) {
			return false;
		}
	}
	return true;

}

bool get_integer(char *input, int *value, char opt) {
	long long long_long_i;
	if (sscanf(input, "%lld", &long_long_i) != 1) {
		return false;
	}
	*value = (int)long_long_i;
	if (long_long_i != (long long)*value) {
		fprintf(stderr, "Error: Integer overflow for for parameter '-%c'.\n", opt);
		return false;
	}
	return true;
}

bool parse_arg(char* optarg, int *val, char opt) {
	if (optarg == NULL) {
		fprintf(stderr, "Error: Option -%c requires an argument.\n", opt);
		return false;
	}
	if (!is_integer(optarg)) {
		fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg, opt);
		return false;
	} else if (!get_integer(optarg, val, opt)) {
		return false;
	}
	return true;
}


int main(int argc, char **argv) {
	
	if (argc == 1) {
		display_usage(argv[0]);
		return EXIT_FAILURE;
	}
	int start, end, num_threads;
	int s_flag = 0, e_flag = 0, t_flag = 0;
	int opts;

	while ((opts = getopt(argc, argv, ":s:e:t:")) != -1) {
		switch(opts) {
			case 's':
				s_flag = 1;
				if (parse_arg(optarg, &start, 's') == false) {
					return EXIT_FAILURE;
				}
				break;
			case 'e':
				e_flag = 1;
				if (parse_arg(optarg, &end, 'e') == false) {
					return EXIT_FAILURE;
				}
				break;
			case 't':
				t_flag = 1;
				if (parse_arg(optarg, &num_threads, 't') == false) {
					return EXIT_FAILURE;
				}
				break;
			case ':':
				if (optopt == 's' || optopt == 'e' || optopt == 't') {
		        	fprintf (stderr, "Error: Option -%c requires an argument.\n", optopt);
		        	return EXIT_FAILURE;
		        }
		    case '?':
		        if (isprint (optopt)) {
		        	fprintf (stderr, "Error: Unknown option '-%c'.\n", optopt);
		        	return EXIT_FAILURE;
		        } else {
		        	fprintf (stderr,"Error: Unknown option character `\\x%x'.\n",optopt);
		        	return EXIT_FAILURE;
		        }
		}
	}
	if (argv[optind] != NULL) {
		fprintf(stderr, "Error: Non-option argument '%s' supplied.\n", argv[optind]);
		return EXIT_FAILURE;
	}
	if (s_flag == 0) {
		fprintf(stderr, "Error: Required argument <starting value> is missing.\n");
		return EXIT_FAILURE;
	}
	if (start < 2) {
		fprintf(stderr, "Error: Starting value must be >= 2.\n");
		return EXIT_FAILURE;
	}
	if (e_flag == 0) {
		fprintf(stderr, "Error: Required argument <ending value> is missing.\n");
		return EXIT_FAILURE;
	}
	if (end < 2) {
		fprintf(stderr, "Error: Ending value must be >= 2.\n");
		return EXIT_FAILURE;
	}
	if (end < start) {
		fprintf(stderr, "Error: Ending value must be >= starting value.\n");
		return EXIT_FAILURE;
	}
	if (t_flag == 0) {
		fprintf(stderr, "Error: Required argument <num threads> is missing.\n");
		return EXIT_FAILURE;
	}
	if (num_threads < 1) {
		fprintf(stderr, "Error: Number of threads cannot be less than 1.\n");
		return EXIT_FAILURE;
	}
	int proc = get_nprocs();
	if (num_threads > (proc * 2)) {
		fprintf(stderr, "Number of threads cannot exceed twice the number of processors (%d).\n", proc);
		return EXIT_FAILURE;
	}
	int n_test = end - start + 1;
	if (n_test < num_threads) {
		num_threads = n_test;
	}

	pthread_t threads[num_threads];
    thread_args targs[num_threads];

    int segment = n_test/num_threads;
    int rem = n_test % num_threads;
    for (int i = 0; i < num_threads; ++i) {
    	if (i == 0) {
    		targs[i].start = start;
    		targs[i].end = start + segment - 1;
    		if (rem == 0) {
    			segment--;
   			}
    	} else if (i == num_threads - 1) {
    		targs[i].start = targs[i - 1].end + 1;
    		targs[i].end = end;
    	} else {
    		targs[i].start = targs[i - 1].end + 1;
    		targs[i].end = targs[i].start + segment - 1;
    	}
    	if (rem > 0) {
    		targs[i].end++;
    		rem--;
    	}	
    }
	printf("Finding all prime numbers between %d and %d.\n", start, end);
	printf("%d segments:\n", num_threads);
     for (int i = 0; i < num_threads; ++i) {
     	printf("	[%d, %d]\n", targs[i].start, targs[i].end);
     }
	 int retval;
	 for(int i = 0; i < num_threads; i++){
	 	if ((retval = pthread_create(&threads[i], NULL, seg_sieve, &targs[i])) != 0) {
          	  fprintf(stderr, "Error: Cannot create thread %d. %s.\n", i+1, strerror(retval));
          	  return EXIT_FAILURE;
        }
	 }
	 for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Warning: Thread %d did not join properly.\n",
                    i + 1);
        }
    }
	printf("Total primes between %d and %d with two or more '3' digits: %d\n",start, end, total_count);
	return EXIT_SUCCESS;
}