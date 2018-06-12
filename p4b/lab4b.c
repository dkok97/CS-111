// NAME: Dinkar Khattar
// EMAIL: dinkarkhattar@ucla.edu
// ID: 204818138

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/time.h>
#include <mraa/aio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define EXIT_OK 0
#define EXIT_ARGS 1
#define EXIT_FAIL 1

#define F 0
#define C 1

char *log_file = 0;
int log_fd;
int scale = F;
int period = 1;
int run_flag = 1;
int debug = 0;
struct tm *now;

mraa_aio_context temp_sensor;
mraa_gpio_context button;

char *cmd_err;
int errno_temp;
void report_error(const char* err)
{
    errno_temp = errno;
    fprintf(stderr, "Error due to: %s.\n", err);
    fprintf(stderr, "Error code: %d.\n", errno_temp);
    fprintf(stderr, "Error message: %s.\n", strerror(errno_temp));
    exit(EXIT_FAIL);
}

void signal_handler(int num)
{
	if (num==SIGINT)
		run_flag=0;
}

void logTime(int fd)
{
    struct timeval clock;
    gettimeofday(&clock, 0);
    now = localtime(&(clock.tv_sec));
    dprintf(fd, "%02d:%02d:%02d ", now->tm_hour, now->tm_min, now->tm_sec);
}

void log_temp_time(int fd, float temp){
    logTime(fd);
    dprintf(fd, "%0.1f\n", temp);
    if (log_file)
    {
      logTime(log_fd);
      dprintf(log_fd, "%0.1f\n", temp);
    }
}

void shut_down()
{
	logTime(1);
	dprintf(1, "%s\n", "SHUTDOWN");
	if (log_file)
	{
		logTime(log_fd);
		dprintf(log_fd, "%s\n", "SHUTDOWN");
	}
	exit(EXIT_OK);
}

void run_command(char* command)
{
	if (strstr(command, "SCALE") != NULL)
	{
		if (strstr(command, "F") != NULL)
		{
			scale=F;
      if (log_file)
      {
        dprintf(log_fd, "SCALE=F\n");
      }
		}
		else
		{
			scale=C;
      if (log_file)
      {
        dprintf(log_fd, "SCALE=C\n");
      }
		}
	}
  if (strstr(command, "PERIOD") != NULL)
	{
		int ch = 0;
		while (command[ch] != 'D')
		{
			ch++;
		}
    ch+=2;
    char* temp = malloc(5*sizeof(char));
    int j=0;
    while (isdigit(command[ch]))
    {
      temp[j] = command[ch];
      ch++;
      j++;
    }
		period = atoi(temp);
    if (log_file)
    {
      dprintf(log_fd, "PERIOD=%i\n", period);
    }
	}
	if (strstr(command, "START") != NULL)
	{
		run_flag = 1;
    if (log_file)
    {
      dprintf(log_fd, "START\n");
    }
	}
	if (strstr(command, "STOP") != NULL)
	{
		run_flag = 0;
    if (log_file)
    {
      dprintf(log_fd, "STOP\n");
    }
	}
  if (strstr(command, "LOG") != NULL)
  {
    int ch = 0;
		while (command[ch] != 'G')
		{
			ch++;
		}
    ch+=2;
    char *message = malloc(256*sizeof(char));
    int j=0;
    while (command[ch] != '\n')
    {
      message[j]=command[ch];
      j++;
      ch++;
    }
    if (log_file)
    {
      dprintf(log_fd, "LOG %.*s\n", j, message);
    }
  }
	if (strstr(command, "OFF") != NULL)
	{
    if (log_file)
    {
      dprintf(log_fd, "OFF\n");
    }
		shut_down();
	}
}

void readTemp_log()
{
	int reading = mraa_aio_read(temp_sensor);
	int B = 4275;     //thermistor value
  float R0 = 100000.0;      //nominal base value
  float R = 1023.0/((float) reading) - 1.0;
  R = R0*R;
  float Cel = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	float Fah = (Cel*9)/5 + 32;
	if (scale==C)
    log_temp_time(1,Cel);
	else
    log_temp_time(1,Fah);
}

int main(int argc, char* argv[])
{
	struct option longopts[] = {
    	{"period", required_argument,  NULL,   'p'},
    	{"scale",  required_argument,  NULL,   's'},
    	{"log",    required_argument,  NULL,   'l'},
      {"debug",  no_argument,        NULL,   'd'},
    	{0, 0, 0, 0}
  	};

  	int ch;

  	while ((ch = getopt_long(argc, argv, "p:s:l:d", longopts, NULL)) != -1) {
		 switch (ch) {
      		  case 'p':
          		period = atoi(optarg);
              break;
          	case 's':
            	if (strcmp("C", optarg) == 0)
              {
                scale = C;
              }
            	break;
          	case 'l':
      	      log_file = optarg;
              log_fd = creat(log_file, 0666);
              if(log_fd < 0)
              {
                cmd_err = "creat";
                report_error(cmd_err);
              }
              break;
            case 'd':
          		debug = 1;
              break;
          	default:
              fprintf(stderr, "Usage: lab4b [--period=#] [--scale=[C/F]] [--log=FILE]\n");
              exit(1);
    	}
  	}

	temp_sensor = mraa_aio_init(1);
  if (!temp_sensor)
  {
    cmd_err = "mraa_aio_init for temp_sensor";
    report_error(cmd_err);
  }

  button = mraa_gpio_init(60);
  if (!button)
  {
    cmd_err = "mraa_aio_init for button";
    report_error(cmd_err);
  }

  mraa_gpio_dir(button, MRAA_GPIO_IN);

  struct pollfd fds[1];
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

  char cmdBuf[2048];

	signal(SIGINT, signal_handler);

  while(1)
  {
    if (poll(fds, 1, 0) < 0)
    {
      cmd_err = "poll";
      report_error(cmd_err);
    }

    if (run_flag)
    {
      readTemp_log();
    }

    if (fds[0].revents & POLLIN)
    {
      int bytes_read = read(0, cmdBuf, sizeof(cmdBuf));
      if (bytes_read < 0)
      {
        cmd_err = "read";
        report_error(cmd_err);
      }

      if (debug) fprintf(stderr, "%s\n", cmdBuf);

      if (bytes_read>0)
      {
        run_command(cmdBuf);
      }
    }

    if (mraa_gpio_read(button) == 1)
    {
      shut_down();
    }

    sleep(period);
  }

	mraa_aio_close(temp_sensor);
  mraa_gpio_close(button);

	return 0;
}
