
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
#include "be.h"

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
	  "  -b backend	backend to use (" DEFAULT_BE_NAME ")\n"
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
	  "  -i pidfile file to write the PID to (none).\n"
	  "  -f		don't fork and stay in the foreground\n"
	  "  -p		enable passive mode, for redundancy purposes (read docs.)\n"
	  "  -r		read-only mode\n"
	  "  -h		show this help\n"
	  "\n"
	  "Available backends: " SUPPORTED_BE "\n"
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
	settings.read_only = 0;
	settings.logfname = NULL;
	settings.pidfile = NULL;
	settings.backend = DEFAULT_BE;

	settings.dbname = strdup(DEFDBNAME);
	settings.logfname = strdup("-");

	while ((c = getopt(argc, argv,
				"b:d:l:L:t:T:u:U:s:S:c:o:i:fprh?")) != -1) {
		switch(c) {
		case 'b':
			settings.backend = be_type_from_str(optarg);
			break;
		case 'd':
			free(settings.dbname);
			settings.dbname = strdup(optarg);
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
			free(settings.logfname);
			settings.logfname = strdup(optarg);
			break;

		case 'i':
			free(settings.pidfile);
			settings.pidfile = strdup(optarg);
			break;

		case 'f':
			settings.foreground = 1;
			break;
		case 'p':
			settings.passive = 1;
			break;
		case 'r':
			settings.read_only = 1;
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

	if (settings.backend == BE_UNKNOWN) {
		printf("Error: unknown backend\n");
		return 0;
	} else if (settings.backend == BE_UNSUPPORTED) {
		printf("Error: unsupported backend\n");
		return 0;
	}

	return 1;
}


static void free_settings()
{
	free(settings.dbname);
	free(settings.logfname);
	free(settings.pidfile);
}


int main(int argc, char **argv)
{
	struct cache *cd;
	struct queue *q;
	struct db_conn *db;
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

	db = db_open(settings.backend, settings.dbname, 0);
	if (db == NULL) {
		errlog("Error opening DB");
		return 1;
	}
	wlog("Opened database \"%s\" with %s backend\n", settings.dbname,
		be_str_from_type(settings.backend));

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

	write_pid();

	dbthread = db_loop_start(db);

	net_loop();

	db_loop_stop(dbthread);

	db->close(db);

	queue_free(q);

	cache_free(cd);

	if (settings.pidfile)
		unlink(settings.pidfile);

	free_settings();

	return 0;
}

