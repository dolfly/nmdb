
#if ENABLE_TIPC

#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <string.h>		/* memcpy() */
#include <unistd.h>		/* close() */

#include <linux/tipc.h>		/* TIPC stuff */

#include "nmdb.h"
#include "net-const.h"
#include "internal.h"
#include "tipc.h"


/* Add a TIPC server to the db connection. Requests will select which server
 * to use by hashing the key. */
int nmdb_add_tipc_server(nmdb_t *db, int port)
{
	int fd;
	struct nmdb_srv *newsrv, *newarray;

	fd = socket(AF_TIPC, SOCK_RDM, 0);
	if (fd < 0)
		return 0;

	newarray = realloc(db->servers,
			sizeof(struct nmdb_srv) * (db->nservers + 1));
	if (newarray == NULL) {
		close(fd);
		return 0;
	}

	db->servers = newarray;
	db->nservers++;

	if (port < 0)
		port = TIPC_SERVER_INST;

	newsrv = &(db->servers[db->nservers - 1]);

	newsrv->fd = fd;
	newsrv->info.tipc.port = port;
	newsrv->info.tipc.srvsa.family = AF_TIPC;
	newsrv->info.tipc.srvsa.addrtype = TIPC_ADDR_NAMESEQ;
	newsrv->info.tipc.srvsa.addr.nameseq.type = TIPC_SERVER_TYPE;
	newsrv->info.tipc.srvsa.addr.nameseq.lower = port;
	newsrv->info.tipc.srvsa.addr.nameseq.upper = port;
	newsrv->info.tipc.srvsa.scope = TIPC_CLUSTER_SCOPE;
	newsrv->info.tipc.srvlen = (socklen_t) sizeof(newsrv->info.tipc.srvsa);

	newsrv->type = TIPC_CONN;

	/* keep the list sorted by port, so we can do a reliable selection */
	qsort(db->servers, db->nservers, sizeof(struct nmdb_srv),
			compare_servers);

	return 1;
}

int tipc_srv_send(struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize)
{
	ssize_t rv;
	rv = sendto(srv->fd, buf, bsize, 0,
			(struct sockaddr *) &(srv->info.tipc.srvsa),
			srv->info.tipc.srvlen);
	if (rv <= 0)
		return 0;
	return 1;
}

/* Used internally to get and parse replies from the server. */
uint32_t tipc_get_rep(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize)
{
	ssize_t rv;
	uint32_t id, reply;

	rv = recv(srv->fd, buf, bsize, 0);
	if (rv < 4 + 4) {
		return -1;
	}

	id = * (uint32_t *) buf;
	id = ntohl(id);
	reply = * ((uint32_t *) buf + 1);
	reply = ntohl(reply);

	if (id != ID_CODE) {
		return -1;
	}

	if (payload != NULL) {
		*payload = buf + 4 + 4;
		*psize = rv - 4 - 4;
	}
	return reply;
}

#else
/* Stubs to use when TIPC is not enabled. */

#include <stdint.h>
#include "nmdb.h"
#include "tipc.h"

int nmdb_add_tipc_server(nmdb_t *db, int port)
{
	return 0;
}

int tipc_srv_send(struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize)
{
	return 0;
}

uint32_t tipc_get_rep(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize)
{
	return -1;
}

#endif /* ENABLE_TIPC */

