#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/aio.h>

sig_atomic_t volatile run_flag=1;

void do_when_inter(int  sig)
{
	if (sig==SIGINT)
		run_flag=0;
}

int main()
{
	uint16_t value;

	mraa_aio_context temp;
	temp=mraa_aio_init(1);

	signal(SIGINT, do_when_inter);

	while (run_flag)
	{
		value=mraa_aio_read(temp);
		printf("%d\n", value);
		usleep(100000);
	}
	mraa_aio_close(temp);
	return 0;
}
