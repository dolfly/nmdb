
/* Global data used throughout the whole application. */

#ifndef _COMMON_H
#define _COMMON_H

/* The cache table */
#include "cache.h"
extern struct cache *cache_table;

/* The queue for database operations */
#include "queue.h"
extern struct queue *op_queue;

/* Settings */
struct settings {
	int tipc_lower;
	int tipc_upper;
	char *tcp_addr;
	int tcp_port;
	char *udp_addr;
	int udp_port;
	char *sctp_addr;
	int sctp_port;
	int numobjs;
	int foreground;
	int passive;
	char *dbname;
	char *logfname;
};
extern struct settings settings;

/* Statistics */
#include "stats.h"
extern struct stats stats;

#endif

