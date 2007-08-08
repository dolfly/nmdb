
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
	int numobjs;
	int foreground;
	int passive;
	char *dbname;
	char *logfname;
};
extern struct settings settings;

/* Statistics */
struct stats {
	unsigned long net_version_mismatch;
	unsigned long net_broken_req;
	unsigned long net_unk_req;
};
extern struct stats stats;
#endif

