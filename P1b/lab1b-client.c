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

char *logfile;
bool log_data = false;
int logfile_fd;
bool isCompress = false;
bool debug = false;
int errno_temp;

struct termios saved_attributes;
char *cmd_err;
int sockfd;

int decompress_and_write(char *buffer, int bytes);
int compress_and_send(char *buffer, int bytes);
void read_and_write(int ifd, int ofd, char *buffer, int bytes);

void report_error (const char* err)
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


void reset_input_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes) < 0)
    {
        cmd_err = "tcsetattr before exit";
        report_error(cmd_err);
    }
}

int decompress_and_write(char *buffer, int bytes)
{
  if (log_data && bytes>0)
  {
    if (dprintf(logfile_fd, "RECEIVED %d bytes: %s \n", bytes, buffer) < 0)
    {
      cmd_err = "dprintf";
      report_error(cmd_err);
    }
  }

  char decompressed_buf[1024]="";

  z_stream strm2;

  strm2.zalloc=Z_NULL;
  strm2.zfree=Z_NULL;
  strm2.opaque=Z_NULL;

  inflateInit(&strm2);

  strm2.avail_in = bytes;
  strm2.next_in = (unsigned char*)buffer;
  strm2.avail_out = sizeof(decompressed_buf);
  strm2.next_out = (unsigned char*)decompressed_buf;

  int ret=0;

  do {
    ret = inflate(&strm2, Z_SYNC_FLUSH);
    if (ret < 0)
    {
      cmd_err = "inflate";
      report_error(cmd_err);
    }
  } while (strm2.avail_in > 0);

  int n = sizeof(decompressed_buf)-strm2.avail_out;

  read_and_write(0, 1, decompressed_buf, n);

  inflateEnd(&strm2);

  return n;
}

int compress_and_send(char *buffer, int bytes)
{
  int i;
  for (i=0; i!=bytes; i++)
  {
    char ch = *(buffer + i);
    switch (ch)
    {
      case 0x04:
        if (debug) fprintf(stderr, " not sure what to do ");
        exit(EXIT_SUCCESS);
      case 0x03:
        if (debug) fprintf(stderr, " not sure what to do ");
        exit(EXIT_SUCCESS);
    }
  }

  char compressed_buf[1024]="";

  z_stream strm;

  strm.zalloc=Z_NULL;
  strm.zfree=Z_NULL;
  strm.opaque=Z_NULL;

  deflateInit(&strm, Z_DEFAULT_COMPRESSION);

  strm.avail_in = bytes;
  strm.next_in = (unsigned char*)buffer;
  strm.avail_out = sizeof(compressed_buf);
  strm.next_out = (unsigned char*)compressed_buf;

  if (debug) fprintf(stderr, " compressing %s with bytes %d ", buffer, bytes);

  int ret = 0;

  do {
    ret = deflate(&strm, Z_SYNC_FLUSH);
    if (ret < 0)
    {
      cmd_err = "deflate";
      report_error(cmd_err);
    }
  } while (strm.avail_in > 0);

  int n = sizeof(compressed_buf)-strm.avail_out;

  if (debug)
  {
    fprintf(stderr, " compressed to %s with bytes %d ", compressed_buf, n);
  }

  deflateEnd(&strm);

  if (log_data && n>0)
  {
    if (dprintf(logfile_fd, "SENT %d bytes: %s \n", n, compressed_buf) < 0)
    {
      cmd_err = "dprintf";
      report_error(cmd_err);
    }
  }

  if (write(sockfd, compressed_buf, n) < 0)
  {
    cmd_err = "compressed write";
    report_error(cmd_err);
  }

  return n;

}

void change_terminal_att (void)
{
    struct termios new_attr;

    /* Make sure stdin is a terminal. */
    if (!isatty (STDIN_FILENO))
    {
        fprintf (stderr, "Not a terminal.\n");
        exit (1);
    }

    /* Save the terminal attributes so we can restore them later. */
    if (tcgetattr (STDIN_FILENO, &saved_attributes) < 0) {
        cmd_err = "tcgetattr";
        report_error(cmd_err);
    }

    /* Set the new terminal modes. */
    tcgetattr (STDIN_FILENO, &new_attr);

    new_attr.c_iflag = ISTRIP;
    new_attr.c_oflag = 0;
    new_attr.c_lflag = 0;
    if (tcsetattr (STDIN_FILENO, TCSANOW, &new_attr) < 0) {
        cmd_err = "tcsetattr";
        report_error(cmd_err);
    }
}

void read_and_write(int ifd, int ofd, char *buffer, int bytes)
{
    if (debug) fprintf(stderr, " writing from %d to %d with bytes %d and message %s ", ifd, ofd, bytes, buffer);
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
                    ret_write = write(ofd, &ch, 1);
                    if (ret_write < 0)
                    {
                        cmd_err = "write";
                        report_error(cmd_err);
                    }
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

void create_connection()
{
    struct pollfd fds[2];

    fds[0].fd = 0;
    fds[0].events = POLLIN | POLLHUP | POLLERR;
    fds[0].revents = 0;

    fds[1].fd = sockfd;
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
            if (debug) fprintf(stderr, " getting keyboard input now ");
            char keyboard_input[1024]="";
            int bytes_read = read(0, keyboard_input, 1024);
            if (bytes_read==0)
            {
              exit(EXIT_SUCCESS);
            }
            if (bytes_read<0)
            {
                cmd_err = "read";
                report_error(cmd_err);
            }

            read_and_write(0, 1, keyboard_input, bytes_read);

            if (isCompress)
            {
              compress_and_send(keyboard_input, bytes_read);
            }
            else
            {
              if (log_data && bytes_read>0)
              {
                if (dprintf(logfile_fd, "SENT %d bytes: ", bytes_read) < 0)
                {
                  cmd_err = "dprintf";
                  report_error(cmd_err);
                }
                read_and_write(0, logfile_fd, keyboard_input, bytes_read);
                dprintf(logfile_fd, "\n");
              }
              read_and_write(0, fds[1].fd, keyboard_input, bytes_read);
            }

        }

        if(fds[1].revents & POLLIN)
        {
            char server_input[1024];
            int bytes_read = read(fds[1].fd, server_input, 1024);
            if (bytes_read==0)
            {
                exit(EXIT_SUCCESS);
            }
            if (bytes_read<0)
            {
                cmd_err = "read";
                report_error(cmd_err);
            }

            if (isCompress)
            {
              decompress_and_write(server_input, bytes_read);
            }
            else
            {
              if (log_data && bytes_read>0)
              {
                if (dprintf(logfile_fd, "RECEIVED %d bytes: ", bytes_read) < 0)
                {
                  cmd_err = "dprintf";
                  report_error(cmd_err);
                }
                read_and_write(0, logfile_fd, server_input, bytes_read);
                dprintf(logfile_fd, "\n");
              }

              read_and_write(0, 1, server_input, bytes_read);
            }
        }

        if (fds[1].revents & (POLLHUP | POLLERR))
        {
            if (close(sockfd) < 0)
            {
                cmd_err = "close for sockfd";
                report_error(cmd_err);
            }
            exit(EXIT_SUCCESS);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    int a=0;
    int portno=0;

    struct option longopts[] = {
        { "port",      required_argument,  0,   'p'},
        { "log",       required_argument,  0,   'l'},
        { "compress",  no_argument,        0,   'c'},
        { "debug",     no_argument,        0,   'd'},
        { 0, 0, 0, 0 }
    };

    while ((a = getopt_long(argc, argv, "p:l:cd", longopts, NULL)) != -1) {
        switch (a)
        {
            case 'p':
                portno = atoi(optarg);
                if (debug) fprintf(stderr, " portno set to %d ", portno);
                break;
            case 'l':
                logfile = optarg;
                log_data = true;
                logfile_fd = creat(logfile, S_IRWXU);
                if (logfile_fd < 0)
                {
                  cmd_err="creat";
                  report_error(cmd_err);
                }
                break;
            case 'c':
                isCompress = true;
                break;
            case 'd':
                debug = true;
                break;
            default:
                fprintf(stderr, "Incorrect Usage.\n");
                fprintf(stderr, "Usage: lab1a-client [--port=port#]\n");
                exit(1);
        }
    }

    if (!portno)
    {
      fprintf(stderr, "Incorrect Usage.\n");
      fprintf(stderr, "Usage: lab1a-client [--port=port#]\n");
      exit(1);
    }

    atexit (reset_input_mode);

    change_terminal_att();

    struct sockaddr_in serv_addr;
    struct hostent *server;

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        cmd_err="socket";
        report_error(cmd_err);
    }

    server = gethostbyname("localhost");

    if (server == NULL) {
        cmd_err="gethostbyname";
        report_error(cmd_err);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (debug) fprintf(stderr, " connecting on %d with sockfd %d ", portno, sockfd);
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cmd_err="connect";
        report_error(cmd_err);
    }

    create_connection();
    return 0;
}
