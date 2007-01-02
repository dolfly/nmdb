
#ifndef _NMDB_H
#define _NMDB_H

#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socklen_t */
#include <linux/tipc.h>		/* struct sockaddr_tipc */

struct nmdb_srv {
	int port;
	int fd;
	struct sockaddr_tipc srvsa;
	socklen_t srvlen;
};

typedef struct nmdb_t {
	unsigned int nservers;
	struct nmdb_srv *servers;
} nmdb_t;

nmdb_t *nmdb_init(int port);
int nmdb_free(nmdb_t *db);

ssize_t nmdb_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
ssize_t nmdb_cache_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);

int nmdb_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);
int nmdb_set_async(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);
int nmdb_cache_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);

int nmdb_del(nmdb_t *db, const unsigned char *key, size_t ksize);
int nmdb_del_async(nmdb_t *db, const unsigned char *key, size_t ksize);
int nmdb_cache_del(nmdb_t *db, const unsigned char *key, size_t ksize);

#endif

