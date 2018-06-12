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

compress_bytes()
{
  z_stream stdin_to_shell;
  z_stream shell_to_stdout;

  stdin_to_shell.zalloc=Z_NULL;
  stdin_to_shell.zfree=Z_NULL;
  stdin_to_shell.opaque=Z_NULL;

  deflateInit(&stdin_to_shell, Z_DEFAULT_COMPRESSION);

  shell_to_stdout.zalloc=Z_NULL;
  shell_to_stdout.zfree=Z_NULL;
  shell_to_stdout.opaque=Z_NULL;

  inflateInit(&shell_to_stdout);

  stdin_to_shell.avail_in = no_of_bytes_in_input_buf;
  stdin_to_shell.next_in = input_buf;
  stdin_to_shell.avail_out = compression_buf_size;
  stdin_to_shell.next_out = compression_buf;

  do {
    deflate(&stdin_to_shell, Z_SYNC_FLUSH);
  } while (stdin_to_shell.avail_in > 0);

  write(shell_fd, compression_buf, compression_buf_size - stdin_to_shell.avail_out);

  shell_to_stdout.avail_in = no_of_bytes_in_input_buf;
  shell_to_stdout.next_in = input_buf;
  shell_to_stdout.avail_out = uncompression_buf_size;
  shell_to_stdout.next_out = uncompression_buf;

  do {
    inflate(&shell_to_stdout, Z_SYNC_FLUSH);
  } while (shell_to_stdout.avail_in > 0);

  inflateEnd(&shell_to_stdout);
  deflateEnd(&stdin_to_shell);

}

int main()
{
  char buffer[100];
  int bytes_read = read(0, buffer, 1024);
  if (bytes_read<0)
  {
      cmd_err = "read";
      report_error(cmd_err);
  }
  write(1, buffer, bytes_read);
}
