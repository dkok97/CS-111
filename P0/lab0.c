/*
NAME: Dinkar Khattar
EMAIL: dinkarkhattar@ucla.edu
ID: 204818138
*/

#include <stdio.h>     /* for I/O functions */
#include <stdlib.h>    /* for exit */
#include <getopt.h>    /* for getopt_long */
#include <signal.h>    /* for catching SIGSEGV */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

void signalHandler(int sig_num) {
    if (sig_num==SIGSEGV)
    {
        char error_message[30] = "Segmentation fault caught. \n";
        write(2, error_message, 30);
        exit(4);
    }
}

int main(int argc, char *argv[]) {

    int seg_fault=0, c=0;

    struct option longopts[] = {
        { "input",      required_argument,  0,   'i'},
        { "output",     required_argument,  0,   'o'},
        { "segfault",   no_argument,        0,   's'},
        { "catch",      no_argument,        0,   'c'},
        { 0, 0, 0, 0 }
    };

    char* input_file = NULL;
    char* output_file = NULL;

    while ((c = getopt_long(argc, argv, "i:o:sc", longopts, NULL)) != -1) {
        switch (c) {
            case 'i':
                input_file=optarg;
                break;
            case 'o':
                output_file=optarg;
                break;
            case 's':
                seg_fault=1;
                break;
            case 'c':
                signal(SIGSEGV, signalHandler);
                break;
             default:
                fprintf(stderr, "Incorrect Usage.\n");
                fprintf(stderr, "Usage: lab0 [--input=input_file] [--output=output_file] [--segfault] [--catch].\n");
                exit(1);
        }
    }

    if (seg_fault==1)
    {
        char* n = NULL;
        char a = *n;
    }

    int errno_temp=0;

    if (input_file) {

        int ifd = open(input_file, O_RDONLY);
        if (ifd >= 0) {
            close(0);
            dup(ifd);
            close(ifd);
        }
        else {
            errno_temp = errno;
            fprintf(stderr, "Input file %s could not be opened \n", input_file);
            fprintf(stderr, "Error code: %d \n", errno_temp);
            fprintf(stderr, "Error message: %s \n", strerror(errno_temp));
            exit(2);
        }
    }

    if (output_file) {

        int ofd = creat(output_file, 0666);
        if (ofd >= 0) {
            close(1);
            dup(ofd);
            close(ofd);
        }
        else {
            errno_temp = errno;
            fprintf(stderr, "Output file %s could not be created \n", output_file);
            fprintf(stderr, "Error code: %d \n", errno_temp);
            fprintf(stderr, "Error message: %s \n", strerror(errno_temp));
            exit(3);
        }
    }

    char *buf[sizeof(char)];

    ssize_t ret = read(0, buf, 1);
    while (ret>0)
    {
        write(1, buf, 1);
        ret = read(0, buf, 1);
    }

    exit(0);

}
