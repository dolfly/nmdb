
#ifndef _STATS_H
#define _STATS_H

/* Statistics structure */
struct stats {
	unsigned long cache_get;
	unsigned long cache_set;
	unsigned long cache_del;
	unsigned long cache_cas;
	unsigned long cache_incr;

	unsigned long db_get;
	unsigned long db_set;
	unsigned long db_del;
	unsigned long db_cas;
	unsigned long db_incr;

	unsigned long cache_hits;
	unsigned long cache_misses;

	unsigned long db_hits;
	unsigned long db_misses;


	unsigned long msg_tipc;
	unsigned long msg_tcp;
	unsigned long msg_udp;
	unsigned long msg_sctp;

	unsigned long net_version_mismatch;
	unsigned long net_broken_req;
	unsigned long net_unk_req;
};

void stats_init(struct stats *s);

#endif

