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
#include <mraa/aio.h>
#include <mraa/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define EXIT_OK 0
#define EXIT_ARGS 1
#define EXIT_FAIL 1

#define F 0
#define C 1

char *log_file = 0;
int log_fd = 0;
int sock_fd = 0;
int scale = F;
int period = 1;
int run_flag = 1;
int debug = 0;
struct tm *now;
int portno = 0;
char *host="localhost";
char *id = "0123456789";
struct hostent* server;
struct sockaddr_in server_addr;

mraa_aio_context temp_sensor;
// mraa_gpio_context button;

char *cmd_err;
int errno_temp;
void report_error_1(const char* err)
{
    errno_temp = errno;
    fprintf(stderr, "Error due to: %s.\n", err);
    fprintf(stderr, "Error code: %d.\n", errno_temp);
    fprintf(stderr, "Error message: %s.\n", strerror(errno_temp));
    exit(1);
}

void report_error_2(const char* err)
{
    errno_temp = errno;
    fprintf(stderr, "Error due to: %s.\n", err);
    fprintf(stderr, "Error code: %d.\n", errno_temp);
    fprintf(stderr, "Error message: %s.\n", strerror(errno_temp));
    exit(2);
}

void signal_handler(int num)
{
	if (num==SIGINT)
		run_flag=0;
}

void logTime(float temp)
{
    struct timeval clock;
    gettimeofday(&clock, 0);
    now = localtime(&(clock.tv_sec));
    if (temp==-1)
    {
      dprintf(log_fd, "%02d:%02d:%02d %s\n", now->tm_hour, now->tm_min, now->tm_sec, "SHUTDOWN");
      dprintf(sock_fd, "%02d:%02d:%02d %s\n", now->tm_hour, now->tm_min, now->tm_sec, "SHUTDOWN");
    }
    else
    {
      dprintf(log_fd, "%02d:%02d:%02d %0.1f\n", now->tm_hour, now->tm_min, now->tm_sec, temp);
      dprintf(sock_fd, "%02d:%02d:%02d %0.1f\n", now->tm_hour, now->tm_min, now->tm_sec, temp);
    }
}

void log_temp_time(float temp)
{
  logTime(temp);
}

void shut_down()
{
	if (1)
	{
		logTime(-1);
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
      if (1)
      {
        dprintf(log_fd, "SCALE=F\n");
      }
		}
		else
		{
			scale=C;
      dprintf(log_fd, "SCALE=C\n");
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
    if (1)
    {
      dprintf(log_fd, "PERIOD=%i\n", period);
    }
	}
	if (strstr(command, "START") != NULL)
	{
		run_flag = 1;
    if (1)
    {
      dprintf(log_fd, "START\n");
    }
	}
	if (strstr(command, "STOP") != NULL)
	{
		run_flag = 0;
    if (1)
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
    if (1)
    {
      dprintf(log_fd, "LOG %.*s\n", j, message);
    }
  }
	if (strstr(command, "OFF") != NULL)
	{
    if (1)
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
    log_temp_time(Cel);
	else
    log_temp_time(Fah);
}

int main(int argc, char* argv[])
{
    struct option longopts[] = {
    	 {"period", required_argument,  NULL,   'p'},
    	 {"scale",  required_argument,  NULL,   's'},
    	 {"log",    required_argument,  NULL,   'l'},
       {"debug",  no_argument,        NULL,   'd'},
       {"id", required_argument,  NULL,   'i'},
    	 {"host",  required_argument,  NULL,   'h'},
    	 {0, 0, 0, 0}
    };

    int ch;

  	while ((ch = getopt_long(argc, argv, "p:s:l:di:h:", longopts, NULL)) != -1)
    {
      switch (ch)
      {
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
                report_error_2(cmd_err);
              }
              break;
            case 'd':
          		debug = 1;
              break;
            case 'i':
              id = optarg;
              break;
            case 'h':
          		host = optarg;
              break;
          	default:
              fprintf(stderr, "Usage: lab4c_tcp [--period=#] [--scale=[C/F]] [--log=FILE] [--host=#] [--id=#] PORTNUM\n");
              exit(1);
    	}
  }

  int port_given = 0;
  if (optind < argc)
  {
    portno = atoi(argv[optind]);
    if (portno<=0)
    {
      fprintf(stderr, "invalid port number%s\n", argv[optind]);
      exit(1);
    }

    port_given = 1;
  }

  if (!port_given)
  {
    fprintf(stderr, "No port specified\n");
    exit(1);
  }
  // else if (strstr(argv[0], "tls"))
  // {
  //   port = TLS_PORT;
  // }
  // else
  // {
  //   port = TCP_PORT;
  // }

  if (!log_file)
  {
    fprintf(stderr, "No log file\n");
    exit(1);
  }

  if (strlen(id) == 0)
  {
    fprintf(stderr, "No ID\n");
    exit(1);
  }

  if (host[0] == '\0')
  {
    fprintf(stderr, "Invalid Host\n");
    exit(1);
  }

	temp_sensor = mraa_aio_init(1);
  if (!temp_sensor)
  {
    cmd_err = "mraa_aio_init for temp_sensor";
    report_error_2(cmd_err);
  }

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    cmd_err="socket";
		report_error_2(cmd_err);
	}

	if ((server = gethostbyname(host)) == NULL)
  {
    cmd_err="gethostbyname";
		report_error_2(cmd_err);
	}

	memset((char*) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy((char*) &server_addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
	server_addr.sin_port = htons(portno);

	if ((connect(sock_fd, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0)
  {
    cmd_err="connect";
		report_error_2(cmd_err);
	}

  dprintf(log_fd, "ID=%s\n", id);
  dprintf(sock_fd, "ID=%s\n", id);

  struct pollfd fds[1];
	fds[0].fd = sock_fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

  char cmdBuf[2048];

  while(1)
  {
    if (poll(fds, 1, 0) < 0)
    {
      cmd_err = "poll";
      report_error_2(cmd_err);
    }

    if (fds[0].revents & POLLIN)
    {
      int bytes_read = read(sock_fd, cmdBuf, sizeof(cmdBuf));
      if (bytes_read < 0)
      {
        cmd_err = "read";
        report_error_2(cmd_err);
      }

      if (debug) fprintf(stderr, "%s\n", cmdBuf);

      if (bytes_read>0)
      {
        run_command(cmdBuf);
      }
    }

    if (run_flag)
    {
      readTemp_log();
    }

    // if (mraa_gpio_read(button) == 1)
    // {
    //   shut_down();
    // }

    sleep(period);
  }

  close(sock_fd);

	mraa_aio_close(temp_sensor);
  // mraa_gpio_close(button);

	return 0;
}
