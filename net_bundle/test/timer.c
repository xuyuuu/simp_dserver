#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>

struct event ev;
struct timeval tv;

void time_cb(int fd, short event, void *argc)
{
	printf("timer wakeup \n");
//	event_add(&ev, &tv);
}

int main(int argc, char **argv)
{
	struct event_base *base;

	base = event_init();

	tv.tv_sec = 10;
	tv.tv_usec = 0;
	evtimer_set(&ev, time_cb, NULL);
	event_add(&ev, &tv);
	event_base_dispatch(base);

	return 0;
}
