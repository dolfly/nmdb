
#include <stdio.h>		/* printf() */
#include <unistd.h>		/* malloc(), fork() and getopt() */
#include <stdlib.h>		/* atoi() */
#include <sys/types.h>		/* for pid_t */
#include <string.h>		/* for strcpy() and strlen() */
#include <pthread.h>		/* for pthread_t */

#include "cache.h"
#include "net.h"
#include "db.h"
#include "common.h"
#include "net-const.h"

#define DEFDBNAME "database"


/* Define the common structures that are used throughout the whole server. */
struct settings settings;
struct stats stats;
struct cache *cache_table;
struct queue *op_queue;


static void help(void) {
	char h[] = \
	  "nmdb [options]\n"
	  "  -d dbpath	database path ('database', must be created with dpmgr)\n"
	  "  -l lower	lower TIPC port number (10)\n"
	  "  -L upper	upper TIPC port number (= lower)\n"
	  "  -a addr	TCP listening address (all local addresses)\n"
	  "  -P port	TCP listening port (26010)\n"
	  "  -c nobj	max. number of objects to be cached, in thousands (128)\n"
	  "  -f		don't fork and stay in the foreground\n"
	  "  -p		enable passive mode, for redundancy purposes (read docs.)\n"
	  "  -h		show this help\n"
	  "\n"
	  "Please report bugs to Alberto Bertogli (albertito@gmail.com)\n"
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
	settings.numobjs = -1;
	settings.foreground = 0;
	settings.passive = 0;

	settings.dbname = malloc(strlen(DEFDBNAME) + 1);
	strcpy(settings.dbname, DEFDBNAME);

	while ((c = getopt(argc, argv, "d:l:L:a:P:c:fph?")) != -1) {
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
		case 'a':
			settings.tcp_addr = optarg;
			break;
		case 'P':
			settings.tcp_port = atoi(optarg);
			break;
		case 'c':
			settings.numobjs = atoi(optarg) * 1024;
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
	if (settings.numobjs == -1)
		settings.numobjs = 128 * 1024;

	return 1;
}


static void init_stats(void)
{
	stats.net_version_mismatch = 0;
	stats.net_broken_req = 0;
	stats.net_unk_req = 0;
	return;
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

	init_stats();

	cd = cache_create(settings.numobjs, 0);
	if (cd == NULL) {
		perror("Error creating cache");
		return 1;
	}
	cache_table = cd;

	q = queue_create();
	if (q == NULL) {
		perror("Error creating queue");
		return 1;
	}
	op_queue = q;

	db = db_open(settings.dbname, 0);
	if (db == NULL) {
		perror("Error opening DB");
		return 1;
	}

	if (!settings.foreground) {
		pid = fork();
		if (pid > 0) {
			/* parent exits */
			return 0;
		} else if (pid < 0) {
			perror("Error in fork()");
			return 1;
		}

		close(0);
		setsid();
	}

	dbthread = db_loop_start(db);

	net_loop();

	db_loop_stop(dbthread);

	db_close(db);

	queue_free(q);

	cache_free(cd);

	return 0;
}

