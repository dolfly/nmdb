
#include <signal.h>		/* signal constants */
#include <stdio.h>		/* perror() */
#include <stdlib.h>		/* exit() */

/* Workaround for libevent 1.1a: the header assumes u_char is typedef'ed to an
 * unsigned char, and that "struct timeval" is in scope. */
typedef unsigned char u_char;
#include <sys/time.h>
#include <event.h>

#include "common.h"
#include "tipc.h"
#include "tcp.h"
#include "net.h"


static void exit_sighandler(int fd, short event, void *arg)
{
	printf("Got signal! Puf!\n");
	event_loopexit(NULL);
}

static void passive_to_active_sighandler(int fd, short event, void *arg)
{
	printf("Passive toggle!\n");
	settings.passive = !settings.passive;
}

void net_loop(void)
{
	int tipc_fd = -1;
	int tcp_fd = -1;
	struct event tipc_evt, tcp_evt, sigterm_evt, sigint_evt, sigusr2_evt;

	event_init();

	/* ENABLE_TIPC and ENABLE_TCP are preprocessor constants defined on
	 * the command line by make. */

	if (ENABLE_TIPC) {
		tipc_fd = tipc_init();
		if (tipc_fd < 0) {
			perror("Error initializing TIPC");
			exit(1);
		}

		event_set(&tipc_evt, tipc_fd, EV_READ | EV_PERSIST, tipc_recv,
				&tipc_evt);
		event_add(&tipc_evt, NULL);
	}

	if (ENABLE_TCP) {
		tcp_fd = tcp_init();
		if (tcp_fd < 0) {
			perror("Error initializing TCP");
			exit(1);
		}

		event_set(&tcp_evt, tcp_fd, EV_READ | EV_PERSIST, tcp_newconnection,
				&tcp_evt);
		event_add(&tcp_evt, NULL);
	}

	signal_set(&sigterm_evt, SIGTERM, exit_sighandler, &sigterm_evt);
	signal_add(&sigterm_evt, NULL);
	signal_set(&sigint_evt, SIGINT, exit_sighandler, &sigint_evt);
	signal_add(&sigint_evt, NULL);
	signal_set(&sigusr2_evt, SIGUSR2, passive_to_active_sighandler,
			&sigusr2_evt);
	signal_add(&sigusr2_evt, NULL);

	event_dispatch();

	event_del(&tipc_evt);
	event_del(&tcp_evt);
	signal_del(&sigterm_evt);
	signal_del(&sigint_evt);
	signal_del(&sigusr2_evt);

	tipc_close(tipc_fd);
	tcp_close(tcp_fd);
}


