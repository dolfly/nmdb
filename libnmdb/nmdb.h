
#ifndef _NMDB_H
#define _NMDB_H

#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socklen_t */
#include <linux/tipc.h>		/* struct sockaddr_tipc */
#include <netinet/in.h>		/* struct sockaddr_in */


struct nmdb_srv {
	int fd;
	int type;
	union {
		struct {
			unsigned int port;
			struct sockaddr_tipc srvsa;
			socklen_t srvlen;
		} tipc;
		struct {
			struct sockaddr_in srvsa;
			socklen_t srvlen;
		} tcp;
		struct {
			struct sockaddr_in srvsa;
			socklen_t srvlen;
		} udp;
	} info;
};

typedef struct nmdb_t {
	unsigned int nservers;
	struct nmdb_srv *servers;
} nmdb_t;

nmdb_t *nmdb_init();
int nmdb_add_tipc_server(nmdb_t *db, int port);
int nmdb_add_tcp_server(nmdb_t *db, const char *addr, int port);
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


#endif

