
#include "stats.h"

void stats_init(struct stats *s)
{
	s->cache_get = 0;
	s->cache_set = 0;
	s->cache_del = 0;
	s->cache_cas = 0;
	s->cache_incr = 0;

	s->db_get = 0;
	s->db_set = 0;
	s->db_del = 0;
	s->db_cas = 0;
	s->db_incr = 0;

	s->cache_misses = 0;
	s->cache_hits = 0;

	s->msg_tipc = 0;
	s->msg_tcp = 0;
	s->msg_udp = 0;
	s->msg_sctp = 0;

	s->net_version_mismatch = 0;
	s->net_broken_req = 0;
	s->net_unk_req = 0;
}


