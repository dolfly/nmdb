
#include <signal.h>		/* signal constants */
#include <stdio.h>		/* perror() */
#include <stdlib.h>		/* exit() */

/* Workaround for libevent 1.1a: the header assumes u_char is typedef'ed to an
 * unsigned char, and that "struct timeval" is in scope. */
typedef unsigned char u_char;
#include <sys/time.h>
#include <event.h>

#include "tipc.h"


static void signal_handler(int fd, short event, void *arg)
{
	printf("Got signal! Puf!\n");
	event_loopexit(NULL);
}


void net_loop(void)
{
	int tipc_fd;
	struct event srv_evt, sigterm_evt, sigint_evt;

	tipc_fd = tipc_init();
	if (tipc_fd < 0) {
		perror("Error initializing TIPC");
		exit(1);
	}

	event_init();

	event_set(&srv_evt, tipc_fd, EV_READ | EV_PERSIST, tipc_recv,
			&srv_evt);
	event_add(&srv_evt, NULL);

	signal_set(&sigterm_evt, SIGTERM, signal_handler, &sigterm_evt);
	signal_add(&sigterm_evt, NULL);
	signal_set(&sigint_evt, SIGINT, signal_handler, &sigint_evt);
	signal_add(&sigint_evt, NULL);

	event_dispatch();
}


