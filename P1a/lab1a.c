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

typedef int bool;
#define true 1
#define false 0

struct termios saved_attributes;
bool shell = false;
bool debug = false;
int errno_temp = 0;
char *cmd_err;
pid_t child = 0;
int pipe1[2];
int pipe2[2];

void signalHandler(int sig_num) {
    if (sig_num==SIGPIPE)
    {
        exit(EXIT_SUCCESS);
    }
}

void report_error (const char* err)
{
    errno_temp = errno;
    fprintf(stderr, "Error due to: %s \n", err);
    fprintf(stderr, "Error code: %d \n", errno_temp);
    fprintf(stderr, "Error message: %s \n", strerror(errno_temp));
    exit(EXIT_FAILURE);
}

void wait_and_childStatus ()
{
  int status;
  if (waitpid(child, &status, 0) < 0)
  {
    cmd_err = "waitpid";
    report_error(cmd_err);
  }
  if (shell) fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
}

void reset_input_mode()
{
  if (shell)
  {
    wait_and_childStatus();
  }
  if (tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes) < 0)
  {
    cmd_err = "tcsetattr before exit";
    report_error(cmd_err);
  }
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

    atexit (reset_input_mode);

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
    if (debug) fprintf(stderr, "writing from %d to %d", ifd, ofd);

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
                  if (shell)
                  {
                    if (close(pipe1[1]) < 0)
                    {
                      cmd_err = "close for pipe1[1]";
                      report_error(cmd_err);
                    }
                  }
                  exit(EXIT_SUCCESS);
                case 0x03:
                  if (shell)
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

void create_shell()
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

  if (debug) fprintf(stderr, "pipes created");

  child = fork();
  if (child < 0)
  {
      cmd_err = "fork";
      report_error(cmd_err);
  }
  if (debug) fprintf(stderr, "forked");

  if (child!=0)
  {
    if (debug) fprintf(stderr, "in parent");

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

    if (debug) fprintf(stderr, "closed pipes in parent");

    struct pollfd fds[2];

    fds[0].fd = 0;
		fds[0].events = POLLIN;
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
        char keyboard_input[256];
        int bytes_read = read(0, keyboard_input, 256);
        if (bytes_read<0)
        {
          cmd_err = "read";
          report_error(cmd_err);
        }
        read_and_write(0, 1, keyboard_input, bytes_read);
        read_and_write(0, pipe1[1], keyboard_input, bytes_read);
      }

      if(fds[1].revents & POLLIN)
      {
        char shell_input[256];
        int bytes_read = read(fds[1].fd, shell_input, 256);
        if (bytes_read<0)
        {
          cmd_err = "read";
          report_error(cmd_err);
        }
        read_and_write(0, 1, shell_input, bytes_read);
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
    if (debug) fprintf(stderr, "in child");

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

    if (debug) fprintf(stderr, "execvp complete");

  }

}

int main(int argc, char *argv[])
{
    int a=0;

    struct option longopts[] = {
        { "shell",   no_argument,        0,   's'},
        { "debug",   no_argument,        0,   'd'},
        { 0, 0, 0, 0 }
    };

    while ((a = getopt_long(argc, argv, "sd", longopts, NULL)) != -1) {
        switch (a) {
            case 's':
                shell=true;
                break;
            case 'd':
                debug=true;
                break;
            default:
                fprintf(stderr, "Incorrect Usage.\n");
                fprintf(stderr, "Usage: lab1a [--shell] [--debug].\n");
                exit(1);
        }
    }

    change_terminal_att();

    if (shell)
    {
      if (debug) fprintf(stderr, "started with shell");
      signal(SIGPIPE, signalHandler);
      create_shell();
    }

    else
    {
      char buf[256];
      int bytes_read = read(0, buf, 256);
      if (bytes_read<0)
      {
        cmd_err = "read without shell";
        report_error(cmd_err);
      }
      while (bytes_read>0)
      {
        read_and_write(0,1,buf,bytes_read);
        bytes_read = read(0, buf, 256);
      }
    }
    return EXIT_SUCCESS;
}
