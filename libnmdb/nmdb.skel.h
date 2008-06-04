
/* Header for the libnmdb library. */

#ifndef _NMDB_H
#define _NMDB_H

/* Defined to 0 or 1 at libnmdb build time according the build configuration,
 * not to be used externally. */
#define _ENABLE_TIPC ++CONFIG_ENABLE_TIPC++
#define _ENABLE_TCP ++CONFIG_ENABLE_TCP++
#define _ENABLE_UDP ++CONFIG_ENABLE_UDP++
#define _ENABLE_SCTP ++CONFIG_ENABLE_SCTP++


#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socklen_t */

#if _ENABLE_TIPC
#include <linux/tipc.h>		/* struct sockaddr_tipc */
#endif

#if (_ENABLE_TCP || _ENABLE_UDP || _ENABLE_SCTP)
#include <netinet/in.h>		/* struct sockaddr_in */
#endif

struct nmdb_srv {
	int fd;
	int type;
	union {

#if _ENABLE_TIPC
		struct {
			unsigned int port;
			struct sockaddr_tipc srvsa;
			socklen_t srvlen;
		} tipc;
#endif

#if (_ENABLE_TCP || _ENABLE_UDP || _ENABLE_SCTP)
		struct {
			struct sockaddr_in srvsa;
			socklen_t srvlen;
		} in;
#endif

	} info;
};

typedef struct nmdb_t {
	unsigned int nservers;
	struct nmdb_srv *servers;
} nmdb_t;


/* Possible flags, notice it may make no sense to mix them, consult the
 * documentation before doing weird things. Values should be kept in sync with
 * the internal net-const.h */
#define NMDB_CACHE_ONLY 1
#define NMDB_SYNC 2


nmdb_t *nmdb_init();
int nmdb_add_tipc_server(nmdb_t *db, int port);
int nmdb_add_tcp_server(nmdb_t *db, const char *addr, int port);
int nmdb_add_udp_server(nmdb_t *db, const char *addr, int port);
int nmdb_add_sctp_server(nmdb_t *db, const char *addr, int port);
int nmdb_free(nmdb_t *db);

ssize_t nmdb_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
ssize_t nmdb_cache_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);

int nmdb_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);
int nmdb_set_sync(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);
int nmdb_cache_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);

int nmdb_del(nmdb_t *db, const unsigned char *key, size_t ksize);
int nmdb_del_sync(nmdb_t *db, const unsigned char *key, size_t ksize);
int nmdb_cache_del(nmdb_t *db, const unsigned char *key, size_t ksize);

int nmdb_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize);
int nmdb_cache_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize);

int nmdb_incr(nmdb_t *db, const unsigned char *key, size_t ksize,
		                int64_t increment);
int nmdb_cache_incr(nmdb_t *db, const unsigned char *key, size_t ksize,
		                int64_t increment);

int nmdb_stats(nmdb_t *db, unsigned char *buf, size_t bsize,
		unsigned int *nservers, unsigned int *nstats);

#endif

