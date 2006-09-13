
/* Global data used throughout the whole application. */

#ifndef _COMMON_H
#define _COMMON_H

/* The cache table */
#include "cache.h"
struct cache *cache_table;

/* The queue for database operations */
#include "queue.h"
struct queue *op_queue;

/* Settings */
struct  {
	int tipc_lower;
	int tipc_upper;
	int numobjs;
	int foreground;
	int passive;
	char *dbname;
} settings;

/* Statistics */
struct {
	unsigned long net_version_mismatch;
	unsigned long net_broken_req;
	unsigned long net_unk_req;
} stats;
#endif

