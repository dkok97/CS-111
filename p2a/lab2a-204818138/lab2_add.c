/*
 NAME: Dinkar Khattar
 EMAIL: dinkarkhattar@ucla.edu
 ID: 204818138
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define NO_LOCK 122             //ASCII code for 'z' (used as a placeholder)
#define SPIN_LOCK 115           //ASCII code for 's'
#define MUTEX_LOCK 109          //ASCII code for 'm'
#define COMPARE_AND_SWAP 90     //ASCII code for 'c'


int nthreads = 0;
long long iterations = 0;
int lock_type = 0;
int opt_yield = 0;
int spin_lock_var = 0;
pthread_mutex_t mutex_var;

char *cmd_err;
int errno_temp;
void report_error(const char* err)
{
    if (strcmp("inflate",err)==0 || strcmp("deflate",err)==0)
    {
        exit(EXIT_FAILURE);
    }
    errno_temp = errno;
    fprintf(stderr, "Error due to: %s. ", err);
    fprintf(stderr, "Error code: %d. ", errno_temp);
    fprintf(stderr, "Error message: %s. ", strerror(errno_temp));
    exit(EXIT_FAILURE);
}

void add(long long *pointer, long long value)
{
    long long sum = *pointer + value;
    if (opt_yield)
      sched_yield();
    *pointer = sum;
}

void add_compare_and_swap(long long *pointer, long long value)
{
  long long actual;
  long long sum;
  do {

    actual = *pointer;
    sum = actual + value;

    if(opt_yield)
      sched_yield();

  } while(__sync_val_compare_and_swap(pointer, actual, sum) != actual);
}

void* thread_function(void *counter)
{
    switch(lock_type)
    {
        case NO_LOCK:
        {
            long i;

            for(i=0; i<iterations; i++)
            {
                add((long long *) counter, 1);
            }

            for(i=0; i<iterations; i++)
            {
                add((long long *) counter, -1);
            }

            break;
        }

        case MUTEX_LOCK:
        {
            long i;

            for(i=0; i<iterations; i++)
            {
                pthread_mutex_lock(&mutex_var);
                add((long long *) counter, 1);
                pthread_mutex_unlock(&mutex_var);
            }

            for(i=0; i<iterations; i++)
            {
                pthread_mutex_lock(&mutex_var);
                add((long long *) counter, -1);
                pthread_mutex_unlock(&mutex_var);
            }

            break;
        }

        case SPIN_LOCK:
        {
            long i;

            for(i=0; i<iterations; i++)
            {
                while(__sync_lock_test_and_set(&spin_lock_var, 1));
                add((long long *) counter, 1);
                __sync_lock_release(&spin_lock_var);
            }

            for(i=0; i<iterations; i++)
            {
                while(__sync_lock_test_and_set(&spin_lock_var, 1));
                add((long long *) counter, -1);
                __sync_lock_release(&spin_lock_var);
            }

            break;
        }

        case COMPARE_AND_SWAP:
        {
            long i;

            for(i=0; i<iterations; i++)
            {
              add_compare_and_swap((long long *) counter, 1);
            }

            for(i=0; i<iterations; i++)
            {
              add_compare_and_swap((long long *) counter, -1);
            }

            break;
        }
    }

    return NULL;
}

void print_results(char lock_test, long long time_in_nanoseconds, long long counter)
{
    long long total_op = nthreads*iterations*2;
    if (lock_test=='z')
    {
        printf("add%s-%s,%d,%lld,%lld,%lld,%lld,%lld\n", opt_yield == 0 ? "":"-yield", "none", nthreads,
               iterations, total_op, time_in_nanoseconds, (time_in_nanoseconds/total_op), counter);
    }
    else
    {
        printf("add%s-%c,%d,%lld,%lld,%lld,%lld,%lld\n", opt_yield == 0 ? "":"-yield", lock_test, nthreads,
               iterations, total_op, time_in_nanoseconds, (time_in_nanoseconds/total_op), counter);
    }
}

int main(int argc, char *argv[])
{
    int c=0;
    lock_type = NO_LOCK;

    struct option longopts[] = {
        { "threads",     required_argument,  0,   't'},
        { "iterations",  required_argument,  0,   'i'},
        { "yield",       no_argument,        0,   'y'},
        { "sync",        required_argument,  0,   's'},
        { 0, 0, 0, 0 }
    };

    while ((c = getopt_long(argc, argv, "t:i:ys", longopts, NULL)) != -1) {
        switch (c) {
            case 't':
                nthreads=atoi(optarg);
                break;
            case 'i':
                iterations=strtoll(optarg, NULL, 10);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                if (strlen(optarg)==1)
                {
                    lock_type = *optarg;
                }
                else
                {
                    fprintf(stderr, "Incorrect Usage.\n");
                    fprintf(stderr, "Usage: lab2a [--threads=#] [--iterations=#] [--yield] [--sync={s,m,c}].\n");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Incorrect Usage.\n");
                fprintf(stderr, "Usage: lab2a [--threads=#] [--iterations=#] [--yield] [--sync={s,m,c}].\n");
                exit(1);
        }
    }

    long long counter = 0;

    if (pthread_mutex_init(&mutex_var, NULL))
    {
      cmd_err = "pthread_mutex_init";
      report_error(cmd_err);
    }

    struct timespec start_time;
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) < 0)
    {
        cmd_err = "clock_gettime";
        report_error(cmd_err);
    }

    pthread_t threads[nthreads];
    long i;

    for (i=0; i<nthreads; i++)
    {
        int ret = pthread_create(&threads[i], NULL, thread_function, &counter);
        if (ret)
        {
            cmd_err = "pthread_create";
            report_error(cmd_err);
        }
    }

    for (i=0; i<nthreads; i++)
    {
        int ret = pthread_join(threads[i], NULL);
        if (ret)
        {
            cmd_err = "pthread_join";
            report_error(cmd_err);
        }
    }

    struct timespec end_time;
    if (clock_gettime(CLOCK_MONOTONIC, &end_time) < 0)
    {
        cmd_err = "clock_gettime";
        report_error(cmd_err);
    }


    long long time_in_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000;
    time_in_ns += end_time.tv_nsec;
    time_in_ns -= start_time.tv_nsec;

    print_results(lock_type, time_in_ns, counter);

    pthread_exit(NULL);
    return 0;
}
