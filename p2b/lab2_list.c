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
#include "SortedList.h"

#define NO_LOCK 122             //ASCII code for 'z' (used as a placeholder)
#define SPIN_LOCK 115           //ASCII code for 's'
#define MUTEX_LOCK 109          //ASCII code for 'm'
#define key_len 11

typedef struct sub_list{
  SortedList_t* list;
  int spin_lock_var;
  pthread_mutex_t mutex_var;
} sub_list;
sub_list* all_lists;

int nthreads = 1;
int nlists=1;
long long total_lock_time_in_ns;
long long iterations = 1;
int lock_type = 0;
int opt_yield = 0;
// int spin_lock_var = 0;
// pthread_mutex_t mutex_var;

int debug = 0;
char *cmd_err;
int errno_temp;
void report_error(const char* err)
{
    if (strcmp("inflate",err)==0 || strcmp("deflate",err)==0)
    {
        exit(EXIT_FAILURE);
    }
    errno_temp = errno;
    fprintf(stderr, "Error due to: %s.\n", err);
    fprintf(stderr, "Error code: %d.\n", errno_temp);
    fprintf(stderr, "Error message: %s.\n", strerror(errno_temp));
    exit(EXIT_FAILURE);
}

void list_corrupted(const char* err)
{
    fprintf(stderr, "Error: List corrupted. Message: %s.\n", err);
    exit(2);
}

void signal_handler(int num)
{
    if (num == SIGSEGV)
    {
        errno_temp = num;
        fprintf(stderr, "Segmentation fault has occured\n");
        fprintf(stderr, "Error code: %d.\n", errno_temp);
        fprintf(stderr, "Error message: %s.\n", strerror(errno_temp));
        exit(2);
    }
}

long long time_elapsed(struct timespec start, struct timespec end)
{
  long long time_in_ns = (end.tv_sec - start.tv_sec) * 1000000000;
  time_in_ns += end.tv_nsec;
  time_in_ns -= start.tv_nsec;
  return time_in_ns;
}

unsigned long hash_function(const char *cp)
{
  size_t hash = 0;
  while (*cp)
      hash = (hash * 10) + *cp++ - '0';
  return hash;
}

void lock(long h)
{
  struct timespec start;
  struct timespec end;
  switch(lock_type)
  {
    case NO_LOCK:
      break;

    case SPIN_LOCK:
      if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
      {
        cmd_err = "clock_gettime";
        report_error(cmd_err);
      }

      while (__sync_lock_test_and_set(&all_lists[h].spin_lock_var, 1) == 1);

      if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
      {
        cmd_err = "clock_gettime";
        report_error(cmd_err);
      }
      total_lock_time_in_ns += time_elapsed(start, end);
      break;

    case MUTEX_LOCK:
      if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
      {
        cmd_err = "clock_gettime";
        report_error(cmd_err);
      }

      pthread_mutex_lock(&all_lists[h].mutex_var);

      if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
      {
        cmd_err = "clock_gettime";
        report_error(cmd_err);
      }
      total_lock_time_in_ns += time_elapsed(start, end);
      break;

    default:
      break;
  }
}

void unlock(long h)
{
  switch(lock_type)
  {
    case NO_LOCK:
      break;
    case SPIN_LOCK:
      __sync_lock_release(&all_lists[h].spin_lock_var);
      break;
    case MUTEX_LOCK:
      pthread_mutex_unlock(&all_lists[h].mutex_var);
      break;
    default:
      break;
  }
}

void* thread_function(void *elements)
{
    SortedListElement_t* curr = elements;

    long long i;
    long h;
    for(i = 0; i < iterations; i++)
    {
        h=hash_function(curr[i].key)%nlists;
        if (debug) {fprintf(stderr, "inserting into sub-list %zu\n", h);}
        lock(h);
        SortedList_insert(all_lists[h].list, curr+i);
        unlock(h);
    }

    int len=0;
    for(i = 0; i < nlists; i++)
    {
        lock(i);
        int sub_l=SortedList_length((all_lists[i]).list);
        if (sub_l < 0)
        {
            cmd_err = "Error while calculating length";
            list_corrupted(cmd_err);
        }
        len+=sub_l;
        unlock(i);
    }

    for(i = 0; i < iterations; i++)
    {
        h=hash_function(curr[i].key)%nlists;
        lock(h);
        SortedListElement_t* a = SortedList_lookup(all_lists[h].list, curr[i].key);
        if (debug) {fprintf(stderr, "looking up in sub-list %zu\n", h);}
        if(a == NULL)
        {
            cmd_err = "Error while looking up elements";
            list_corrupted(cmd_err);
        }

        int ret = SortedList_delete(a);

        if(ret == 1)
        {
            cmd_err = "Error while deleting elements";
            list_corrupted(cmd_err);
        }
        unlock(h);
    }

    return NULL;
}

void print_results(char lock_test, long long time_in_nanoseconds)
{
    long long total_op = nthreads*iterations*3;
    long long total_lock_op= nthreads*(2*iterations + nlists);
    char *opts;
    if (opt_yield==0)
      opts="none";
    else if (opt_yield & INSERT_YIELD)
    {
      if (opt_yield & DELETE_YIELD)
      {
        if (opt_yield & LOOKUP_YIELD)
          opts="idl";
        else
          opts="id";
      }
      else if (opt_yield & LOOKUP_YIELD)
        opts="il";
      else
        opts="i";
    }
    else if (opt_yield & DELETE_YIELD)
    {
      if (opt_yield & LOOKUP_YIELD)
        opts="dl";
      else
        opts="d";
    }
    else
      opts="l";

    if (lock_test=='z')
    {
        printf("list-%s-%s,%d,%lld,%d,%lld,%lld,%lld,%d\n", opts, "none", nthreads,
               iterations, nlists, total_op, time_in_nanoseconds, (time_in_nanoseconds/total_op), 0);
    }
    else
    {
        printf("list-%s-%c,%d,%lld,%d,%lld,%lld,%lld,%lld\n", opts, lock_test, nthreads,
               iterations, nlists, total_op, time_in_nanoseconds, (time_in_nanoseconds/total_op), total_lock_time_in_ns/total_lock_op);
    }
}

int main(int argc, char *argv[])
{
    int c=0;
    lock_type = NO_LOCK;
    char *yield_opt = "none";
    int y_o_given = 0;
    int i;

    struct option longopts[] = {
        { "threads",     required_argument,  0,   't'},
        { "iterations",  required_argument,  0,   'i'},
        { "yield",       required_argument,  0,   'y'},
        { "sync",        required_argument,  0,   's'},
        { "lists",       required_argument,  0,   'l'},
        { "debug",       no_argument,        0,   'd'},
        { 0, 0, 0, 0 }
    };

    while ((c = getopt_long(argc, argv, "t:i:ysld", longopts, NULL)) != -1) {
        switch (c) {
            case 't':
                nthreads=atoi(optarg);
                break;
            case 'i':
                iterations=strtoll(optarg, NULL, 10);
                break;
            case 'y':
                yield_opt=optarg;
                y_o_given=1;
                break;
            case 's':
                if (strlen(optarg)==1)
                {
                    lock_type = *optarg;
                }
                else
                {
                    fprintf(stderr, "Incorrect Usage.\n");
                    fprintf(stderr, "Usage: lab2_list [--threads=#] [--iterations=#] [--yield] [--sync={s,m,c}] [--lists=#].\n");
                    exit(1);
                }
                break;
            case 'l':
                nlists=atoi(optarg);
                break;
            case 'd':
                debug=1;
                break;
            default:
                fprintf(stderr, "Incorrect Usage.\n");
                fprintf(stderr, "Usage: lab2_list [--threads=#] [--iterations=#] [--yield] [--sync={s,m,c}] [--lists=#].\n");
                exit(1);
        }
    }

    if (y_o_given)
    {
    size_t j;
    char ch;
    for (j=0; j < strlen(yield_opt); j++)
    {
        ch = *(yield_opt + j);
        if (ch=='i')
        {
            opt_yield |= INSERT_YIELD;
            if (debug) fprintf(stderr, "insert set\n");
        }
        else if (ch=='d')
        {
            opt_yield |= DELETE_YIELD;
            if (debug) fprintf(stderr, "delete set\n");
        }
        else if (ch=='l')
        {
            opt_yield |= LOOKUP_YIELD;
            if (debug) fprintf(stderr, "lookup set\n");
        }
        else
        {
            fprintf(stderr, "Incorrect Usage.\n");
            fprintf(stderr, "Usage: lab2_list [--threads=#] [--iterations=#] [--yield] [--sync={s,m,c}] [--lists=#].\n");
            exit(1);
        }

    }
    }

    signal(SIGSEGV, signal_handler);

    long long num_elements = nthreads * iterations;

    all_lists = malloc(sizeof(sub_list)*nlists);
    if (all_lists==NULL)
    {
        cmd_err = "malloc";
        report_error(cmd_err);
    }
    for(i=0;i<nlists;i++)
    {
      all_lists[i].list=malloc(sizeof(SortedList_t));
      if (all_lists[i].list==NULL)
      {
          cmd_err = "malloc";
          report_error(cmd_err);
      }
      all_lists[i].list->key=NULL;
      all_lists[i].list->next=all_lists[i].list;
      all_lists[i].list->prev=all_lists[i].list;
      if (lock_type==SPIN_LOCK)
        all_lists[i].spin_lock_var=0;
      if (lock_type==MUTEX_LOCK)
      {
        if (pthread_mutex_init(&all_lists[i].mutex_var, NULL))
        {
            cmd_err = "pthread_mutex_init";
            report_error(cmd_err);
        }
      }
    }

    SortedListElement_t *elements = malloc(num_elements * sizeof(SortedListElement_t));

    srand(time(NULL));
    static const char alphanum[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    for(i=0;i<num_elements;i++)
    {
        char *s = malloc((key_len) * sizeof(char));
        int j;
        for(j=0; j<key_len-1; j++)
        {
            s[j] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        s[key_len-1] = '\0';
        elements[i].key = (const char *) s;
    }

    struct timespec start_time;
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) < 0)
    {
        cmd_err = "clock_gettime";
        report_error(cmd_err);
    }

    pthread_t threads[nthreads];
    long j;

    for (j=0; j<nthreads; j++)
    {
        int ret = pthread_create(&threads[j], NULL, thread_function, elements + j*iterations);
        if (ret)
        {
            cmd_err = "pthread_create";
            report_error(cmd_err);
        }
    }

    for (j=0; j<nthreads; j++)
    {
        int ret = pthread_join(threads[j], NULL);
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


    long long time_in_ns = time_elapsed(start_time, end_time);

    int len=0;
    for(i = 0; i < nlists; i++)
    {
        int sub_l=SortedList_length((all_lists[i]).list);
        if (sub_l < 0)
        {
            cmd_err = "Error while calculating length";
            list_corrupted(cmd_err);
        }
        len+=sub_l;
    }
    if(len != 0)
    {
        cmd_err = "length not 0 after execution of threads";
        list_corrupted(cmd_err);
    }

    free(all_lists);
    free(elements);

    print_results(lock_type, time_in_ns);

    pthread_exit(NULL);
    return 0;
}
