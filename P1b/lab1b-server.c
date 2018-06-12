#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include "zlib.h"

typedef int bool;
#define true 1
#define false 0

bool isCompress = false;
bool debug = false;
int errno_temp;

char *cmd_err;
int newsockfd;
pid_t child = 0;
int pipe1[2];
int pipe2[2];

int decompress_and_write(char *buffer, int bytes);
int compress_and_send(char *buffer, int bytes);
void read_and_write(int ifd, int ofd, char *buffer, int bytes);

void signalHandler(int sig_num) {
    if (sig_num==SIGPIPE)
    {
        exit(EXIT_SUCCESS);
    }
}

void report_error (const char* err)
{
    if (strcmp("inflate",err)==0 || strcmp("deflate",err)==0)
    {
      exit(EXIT_FAILURE);
    }
    errno_temp = errno;
    fprintf(stderr, "Error due to: %s \n", err);
    fprintf(stderr, "Error code: %d \n", errno_temp);
    fprintf(stderr, "Error message: %s \n", strerror(errno_temp));
    exit(EXIT_FAILURE);
}

int compress_and_send(char *buffer, int bytes)
{
  char compressed_buf[1024]="";

  z_stream strm2;

  strm2.zalloc=Z_NULL;
  strm2.zfree=Z_NULL;
  strm2.opaque=Z_NULL;

  deflateInit(&strm2, Z_DEFAULT_COMPRESSION);

  strm2.avail_in = bytes;
  strm2.next_in = (unsigned char*)buffer;
  strm2.avail_out = sizeof(compressed_buf);
  strm2.next_out = (unsigned char*)compressed_buf;

  int ret = 0;

  do {
    ret = deflate(&strm2, Z_SYNC_FLUSH);
    if (ret < 0)
    {
      cmd_err = "deflate";
      report_error(cmd_err);
    }
  } while (strm2.avail_in > 0);

  int n = sizeof(compressed_buf)-strm2.avail_out;

  if (write(newsockfd, compressed_buf, n) < 0)
  {
    cmd_err = "write after compress at server";
    report_error(cmd_err);
  }

  deflateEnd(&strm2);

  return n;
}

int decompress_and_write(char *buffer, int bytes)
{
  if (debug) fprintf(stderr, " got compressed %s with bytes %d", buffer, bytes);

  char decompressed_buf[1024]="";

  z_stream strm;

  strm.zalloc=Z_NULL;
  strm.zfree=Z_NULL;
  strm.opaque=Z_NULL;

  inflateInit(&strm);

  strm.avail_in = bytes;
  strm.next_in = (unsigned char*)buffer;
  strm.avail_out = sizeof(decompressed_buf);
  strm.next_out = (unsigned char*)decompressed_buf;

  if (debug) fprintf(stderr, " going to decompress ");

  int ret = 0;

  do {
    ret = inflate(&strm, Z_SYNC_FLUSH);
    if (debug) fprintf(stderr, " in inflate loop with ret %d and avail.in %d and message %s", ret, strm.avail_in, strm.msg);
    if (ret < 0)
    {
      cmd_err = "inflate";
      report_error(cmd_err);
    }
  } while (strm.avail_in > 0);

  int n = sizeof(decompressed_buf)-strm.avail_out;

  if (debug) fprintf(stderr, " actually got %s with bytes %d ", decompressed_buf, n);

  read_and_write(0, pipe1[1], decompressed_buf, n);

  inflateEnd(&strm);

  return n;
}

void read_and_write(int ifd, int ofd, char *buffer, int bytes)
{
    if (debug) fprintf(stderr, " writing from %d to %d with bytes %d", ifd, ofd, bytes);
    int i;
    for (i=0; i!=bytes; i++)
    {
        char ch = *(buffer + i);
        char *cr_lf = "\r\n";
        int ret_write=0;
        switch(ch)
        {
            case '\r':
            case '\n':
                if (ofd==1)
                {
                    ret_write = write(ofd, cr_lf, 2);
                    if (ret_write < 0)
                    {
                        cmd_err = "write";
                        report_error(cmd_err);
                    }
                }
                else
                {
                    ret_write = write(ofd, cr_lf+1, 2);
                    if (ret_write < 0)
                    {
                        cmd_err = "write";
                        report_error(cmd_err);
                    }
                }
                break;
            case 0x04:
                if (close(pipe1[1]) < 0)
                {
                    cmd_err = "close for pipe1[1]";
                    report_error(cmd_err);
                }
                break;
            case 0x03:
                if (kill(child, SIGINT) < 0)
                {
                    cmd_err = "kill when encountered ^C";
                    report_error(cmd_err);
                }
                break;
            default:
                ret_write = write(ofd, &ch, 1);
                if (ret_write < 0)
                {
                    cmd_err = "write";
                    report_error(cmd_err);
                }
        }
    }
}

void wait_and_childStatus ()
{
    int status;
    if (waitpid(child, &status, 0) < 0)
    {
        cmd_err = "waitpid";
        report_error(cmd_err);
    }

    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
}

void receive_and_process()
{
    int pipe_ret = -1;
    pipe_ret = pipe(pipe1);
    if (pipe_ret < 0)
    {
        cmd_err = "pipe to child";
        report_error(cmd_err);
    }
    pipe_ret = pipe(pipe2);
    if (pipe_ret < 0)
    {
        cmd_err = "pipe from child";
        report_error(cmd_err);
    }

    child = fork();
    if (child < 0)
    {
        cmd_err = "fork";
        report_error(cmd_err);
    }

    if (child!=0)
    {
        if (close(pipe1[0]) < 0)
        {
            cmd_err = "close for pipe1[0]";
            report_error(cmd_err);
        }
        if (close(pipe2[1]) < 0)
        {
            cmd_err = "close for pipe2[1]";
            report_error(cmd_err);
        }

        struct pollfd fds[2];

        fds[0].fd = newsockfd;
        fds[0].events = POLLIN | POLLHUP | POLLERR;
        fds[0].revents = 0;

        fds[1].fd = pipe2[0];
        fds[1].events = POLLIN | POLLHUP | POLLERR;
        fds[1].revents = 0;

        while (1)
        {
            int poll_ret = poll(fds, 2, 0);

            if (poll_ret < 0)
            {
                cmd_err = "poll";
                report_error(cmd_err);
            }

            if (poll_ret == 0)
            {
                continue;
            }

            if(fds[0].revents & POLLIN)
            {
                char from_client[1024]="";
                int bytes_read = read(newsockfd, from_client, 1024);
                if (bytes_read<0)
                {
                  cmd_err = "read";
                  report_error(cmd_err);
                }
                if (isCompress)
                {
                  decompress_and_write(from_client, bytes_read);
                }
                else
                {
                  read_and_write(0, pipe1[1], from_client, bytes_read);
                }
            }

            if(fds[1].revents & POLLIN)
            {
                char shell_input[1024]="";
                int bytes_read = read(fds[1].fd, shell_input, 1024);
                if (bytes_read<0)
                {
                    cmd_err = "read";
                    report_error(cmd_err);
                }
                if (isCompress)
                {
                  compress_and_send(shell_input, bytes_read);
                }
                else
                {
                  read_and_write(0, newsockfd, shell_input, bytes_read);
                }
            }

            if (fds[1].revents & (POLLHUP | POLLERR))
            {
                if (close(pipe1[1]) < 0)
                {
                    cmd_err = "close for pipe1[1]";
                    report_error(cmd_err);
                }
                break;
            }

        }

    }

    else if (child==0)
    {
        if (close(pipe1[1]) < 0)
        {
            cmd_err = "close for pipe1[1]";
            report_error(cmd_err);
        }
        if (close(pipe2[0]) < 0)
        {
            cmd_err = "close for pipe2[0]";
            report_error(cmd_err);
        }
        if (dup2(pipe1[0], 0) < 0)
        {
            cmd_err = "dup of pipe1[0]]";
            report_error(cmd_err);
        }
        if (dup2(pipe2[1], 1) < 0)
        {
            cmd_err = "dup of pipe2[1]]";
            report_error(cmd_err);
        }
        if (dup2(pipe2[1], 2) < 0)
        {
            cmd_err = "dup of pipe2[1]]";
            report_error(cmd_err);
        }
        if (close(pipe1[0]) < 0)
        {
            cmd_err = "close for pipe1[0]";
            report_error(cmd_err);
        }
        if (close(pipe2[1]) < 0)
        {
            cmd_err = "close for pipe2[1]";
            report_error(cmd_err);
        }

        char **x = NULL;
        if (execvp("/bin/bash", x) < 0)
        {
            cmd_err = "execvp";
            report_error(cmd_err);
        }

        if (debug) fprintf(stderr, " execvp complete ");

    }

    atexit(wait_and_childStatus);

}

int main( int argc, char *argv[] )
{
    int a=0;
    int portno;

    struct option longopts[] = {
        { "port",   required_argument,     0,   'p'},
        { "compress",  no_argument,        0,   'c'},
        { "debug",     no_argument,        0,   'd'},
        { 0, 0, 0, 0 }
    };

    while ((a = getopt_long(argc, argv, "p:cd", longopts, NULL)) != -1) {
        switch (a) {
            case 'p':
                portno=atoi(optarg);
                break;
            case 'c':
                isCompress = true;
                break;
            case 'd':
                debug = true;
                break;
            default:
                fprintf(stderr, "Incorrect Usage.\n");
                fprintf(stderr, "Usage: lab1a-server [--port=port#]\n");
                exit(1);
        }
    }

    if (!portno)
    {
      fprintf(stderr, "Incorrect Usage.\n");
      fprintf(stderr, "Usage: lab1a-client [--port=port#]\n");
      exit(1);
    }

    int sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        cmd_err = "socket";
        report_error(cmd_err);
    }

    /* Initialize socket structure */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cmd_err = "bind";
        report_error(cmd_err);
    }

    /* Now start listening for the clients, here process will
     * go in sleep mode and will wait for the incoming connection
     */

    if (listen(sockfd,5) < 0)
    {
      cmd_err = "sockfd";
      report_error(cmd_err);
    }
    if (debug) fprintf(stderr, " listening on %d with sockfd %d ", portno, sockfd);
    clilen = sizeof(cli_addr);

    /* Accept actual connection from the client */
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) {
        cmd_err = "accept";
        report_error(cmd_err);
    }

    receive_and_process();

    return 0;
}
