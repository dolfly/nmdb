
#include <stdio.h>		/* printf() */
#include <unistd.h>		/* malloc(), fork() and getopt() */
#include <stdlib.h>		/* atoi() */
#include <sys/types.h>		/* for pid_t */
#include <string.h>		/* for strcpy() and strlen() */
#include <pthread.h>		/* for pthread_t */

#include "cache.h"
#include "net.h"
#include "dbloop.h"
#include "common.h"
#include "net-const.h"
#include "log.h"
#include "stats.h"

#define DEFDBNAME "database"


/* Define the common structures that are used throughout the whole server. */
struct settings settings;
struct stats stats;
struct cache *cache_table;
struct queue *op_queue;


static void help(void) {
	char h[] = \
	  "nmdb [options]\n"
	  "\n"
	  "  -d dbpath	database path ('database' by default)\n"
	  "  -l lower	TIPC lower port number (10)\n"
	  "  -L upper	TIPC upper port number (= lower)\n"
	  "  -t port	TCP listening port (26010)\n"
	  "  -T addr	TCP listening address (all local addresses)\n"
	  "  -u port	UDP listening port (26010)\n"
	  "  -U addr	UDP listening address (all local addresses)\n"
	  "  -s port	SCTP listening port (26010)\n"
	  "  -S addr	SCTP listening address (all local addresses)\n"
	  "  -c nobj	max. number of objects to be cached, in thousands (128)\n"
	  "  -o fname	log to the given file (stdout).\n"
	  "  -f		don't fork and stay in the foreground\n"
	  "  -p		enable passive mode, for redundancy purposes (read docs.)\n"
	  "  -h		show this help\n"
	  "\n"
	  "Please report bugs to Alberto Bertogli (albertito@blitiri.com.ar)\n"
	  "\n";
	printf("%s", h);
}


static int load_settings(int argc, char **argv)
{
	int c;

	settings.tipc_lower = -1;
	settings.tipc_upper = -1;
	settings.tcp_addr = NULL;
	settings.tcp_port = -1;
	settings.udp_addr = NULL;
	settings.udp_port = -1;
	settings.sctp_addr = NULL;
	settings.sctp_port = -1;
	settings.numobjs = -1;
	settings.foreground = 0;
	settings.passive = 0;
	settings.logfname = "-";

	settings.dbname = malloc(strlen(DEFDBNAME) + 1);
	strcpy(settings.dbname, DEFDBNAME);

	while ((c = getopt(argc, argv, "d:l:L:t:T:u:U:s:S:c:o:fph?")) != -1) {
		switch(c) {
		case 'd':
			free(settings.dbname);
			settings.dbname = malloc(strlen(optarg) + 1);
			strcpy(settings.dbname, optarg);
			break;

		case 'l':
			settings.tipc_lower = atoi(optarg);
			break;
		case 'L':
			settings.tipc_upper = atoi(optarg);
			break;

		case 't':
			settings.tcp_port = atoi(optarg);
			break;
		case 'T':
			settings.tcp_addr = optarg;
			break;

		case 'u':
			settings.udp_port = atoi(optarg);
			break;
		case 'U':
			settings.udp_addr = optarg;
			break;

		case 's':
			settings.sctp_port = atoi(optarg);
			break;
		case 'S':
			settings.sctp_addr = optarg;
			break;

		case 'c':
			settings.numobjs = atoi(optarg) * 1024;
			break;

		case 'o':
			settings.logfname = malloc(strlen(optarg) + 1);
			strcpy(settings.logfname, optarg);
			break;

		case 'f':
			settings.foreground = 1;
			break;
		case 'p':
			settings.passive = 1;
			break;

		case 'h':
		case '?':
			help();
			return 0;
		default:
			printf("Unknown parameter '%c'\n", c);
			return 0;
		}
	}

	if (settings.tipc_lower == -1)
		settings.tipc_lower = TIPC_SERVER_INST;
	if (settings.tipc_upper == -1)
		settings.tipc_upper = settings.tipc_lower;
	if (settings.tcp_addr == NULL)
		settings.tcp_addr = TCP_SERVER_ADDR;
	if (settings.tcp_port == -1)
		settings.tcp_port = TCP_SERVER_PORT;
	if (settings.udp_addr == NULL)
		settings.udp_addr = UDP_SERVER_ADDR;
	if (settings.udp_port == -1)
		settings.udp_port = UDP_SERVER_PORT;
	if (settings.sctp_addr == NULL)
		settings.sctp_addr = SCTP_SERVER_ADDR;
	if (settings.sctp_port == -1)
		settings.sctp_port = SCTP_SERVER_PORT;
	if (settings.numobjs == -1)
		settings.numobjs = 128 * 1024;

	return 1;
}


int main(int argc, char **argv)
{
	struct cache *cd;
	struct queue *q;
	db_t *db;
	pid_t pid;
	pthread_t *dbthread;

	if (!load_settings(argc, argv))
		return 1;

	if (!log_init()) {
		perror("Error opening log file");
		return 1;
	}

	stats_init(&stats);

	cd = cache_create(settings.numobjs, 0);
	if (cd == NULL) {
		errlog("Error creating cache");
		return 1;
	}
	cache_table = cd;

	q = queue_create();
	if (q == NULL) {
		errlog("Error creating queue");
		return 1;
	}
	op_queue = q;

	db = db_open(settings.dbname, 0);
	if (db == NULL) {
		errlog("Error opening DB");
		return 1;
	}

	if (!settings.foreground) {
		pid = fork();
		if (pid > 0) {
			/* parent exits */
			return 0;
		} else if (pid < 0) {
			errlog("Error in fork()");
			return 1;
		}

		close(0);
		setsid();
	}

	wlog("Starting nmdb\n");

	dbthread = db_loop_start(db);

	net_loop();

	db_loop_stop(dbthread);

	db_close(db);

	queue_free(q);

	cache_free(cd);

	return 0;
}

