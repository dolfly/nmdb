
#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <linux/tipc.h>		/* tipc stuff */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <string.h>		/* memcpy() */
#include <unistd.h>		/* close() */

#include <stdio.h>

#include "nmdb.h"
#include "net-const.h"

/* The ID code for requests is hardcoded for now, until asynchronous requests
 * are implemented. */
#define ID_CODE 1


/* Create a nmdb_t and set the first server to port. If port is < 0, the
 * standard port is used. */
nmdb_t *nmdb_init(int port)
{
	int fd;
	nmdb_t *db;
	struct nmdb_srv *server;

	db = malloc(sizeof(nmdb_t));
	if (db == NULL) {
		return NULL;
	}

	server = malloc(sizeof(struct nmdb_srv));
	if (server == NULL) {
		free(db);
		return NULL;
	}

	db->servers = server;
	db->nservers = 1;

	/* the fd is shared among the different servers, because we use
	 * sendto() directly instead of connecting sockets */
	fd = socket(AF_TIPC, SOCK_RDM, 0);
	if (fd < 0) {
		free(db->servers);
		free(db);
		return NULL;
	}
	db->fd = fd;

	if (port < 0)
		port = SERVER_INST;

	server->port = port;
	server->srvsa.family = AF_TIPC;
	server->srvsa.addrtype = TIPC_ADDR_NAMESEQ;
	server->srvsa.addr.nameseq.type = SERVER_TYPE;
	server->srvsa.addr.nameseq.lower = port;
	server->srvsa.addr.nameseq.upper = port;
	server->srvsa.scope = TIPC_CLUSTER_SCOPE;
	server->srvlen = (socklen_t) sizeof(server->srvsa);

	return db;
}

/* Compare two servers, using their ports. It is used internally to keep the
 * server array sorted with qsort(). */
static int compare_servers(const void *s1, const void *s2)
{
	struct nmdb_srv *srv1 = (struct nmdb_srv *) s1;
	struct nmdb_srv *srv2 = (struct nmdb_srv *) s2;

	if (srv1->port < srv2->port)
		return -1;
	else if (srv1->port == srv2->port)
		return 0;
	else
		return 1;
}

/* Add a server to the db connection. Requests will select which server to use
 * by hashing the key. */
int nmdb_add_server(nmdb_t *db, int port)
{
	struct nmdb_srv *newsrv, *newarray;

	newarray = realloc(db->servers,
			sizeof(struct nmdb_srv) * (db->nservers + 1));
	if (newarray == NULL) {
		return 0;
	}
	db->servers = newarray;
	db->nservers++;

	newsrv = &(db->servers[db->nservers - 1]);

	if (port < 0)
		port = SERVER_INST;

	newsrv->port = port;
	newsrv->srvsa.family = AF_TIPC;
	newsrv->srvsa.addrtype = TIPC_ADDR_NAMESEQ;
	newsrv->srvsa.addr.nameseq.type = SERVER_TYPE;
	newsrv->srvsa.addr.nameseq.lower = port;
	newsrv->srvsa.addr.nameseq.upper = port;
	newsrv->srvsa.scope = TIPC_CLUSTER_SCOPE;
	newsrv->srvlen = (socklen_t) sizeof(newsrv->srvsa);

	/* keep the list sorted by port, so we can do a reliable selection */
	qsort(db->servers, db->nservers, sizeof(struct nmdb_srv),
			compare_servers);

	return 1;
}


/* Frees a nmdb_t structure created with nmdb_init(). */
int nmdb_free(nmdb_t *db)
{
	close(db->fd);
	if (db->servers != NULL)
		free(db->servers);
	free(db);
	return 1;
}


/* Used internally to send a buffer to the given server. */
static int srv_send(nmdb_t *db, struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize)
{
	ssize_t rv;
	rv = sendto(db->fd, buf, bsize, 0, (struct sockaddr *) &(srv->srvsa),
			srv->srvlen);
	if (rv <= 0)
		return 0;
	return 1;
}

/* Used internally to receive a buffer. */
static ssize_t srv_recv(nmdb_t *db, struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize)
{
	ssize_t rv;
	rv = recv(db->fd, buf, bsize, 0);
	return rv;
}

/* Used internally to get and parse replies from the server. */
static uint32_t get_rep(nmdb_t *db, struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize)
{
	ssize_t t;
	uint32_t id, reply;

	t = srv_recv(db, srv, buf, bsize);
	if (t < 4 + 4) {
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
		*psize = t - 4 - 4;
	}
	return reply;
}

/* Hash function used internally by select_srv(). See RFC 1071. */
uint32_t checksum(const unsigned char *buf, size_t bsize)
{
	uint32_t sum = 0;

	while (bsize > 1)  {
		sum += * (uint16_t *) buf++;
		bsize -= 2;
	}

	if (bsize > 0)
		sum += * (uint8_t *) buf;

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

/* Used internally to select which server to use for the given key. */
static struct nmdb_srv *select_srv(nmdb_t *db,
		const unsigned char *key, size_t ksize)
{
	uint32_t n;

	n = checksum(key, ksize) % db->nservers;
	return &(db->servers[n]);
}


static ssize_t do_get(nmdb_t *db,
		const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize, int impact_db)
{
	ssize_t rv, t;
	unsigned char *buf, *p;
	size_t bsize, reqsize, psize;
	uint32_t request, reply;
	struct nmdb_srv *srv;

	if (impact_db) {
		request = REQ_GET;
	} else {
		request = REQ_CACHE_GET;
	}

	/* Use the same buffer for the request and the reply.
	 * Request: 4 bytes ver+id, 4 bytes request code, 4 bytes ksize,
	 * 		ksize bytes key.
	 * Reply: 4 bytes id, 4 bytes reply code, 4 bytes vsize,
	 * 		vsize bytes key.
	 *
	 * We don't know vsize beforehand, but we do know TIPC's max packet is
	 * 66000. We malloc 128k just in case.
	 */
	bsize = 128 * 1024;
	buf = malloc(bsize);
	if (buf == NULL)
		return -1;

	* (uint32_t *) buf = htonl( (PROTO_VER << 28) | ID_CODE );
	* ((uint32_t *) buf + 1) = htonl(request);
	* ((uint32_t *) buf + 2) = htonl(ksize);
	p = buf + 3 * 4;
	memcpy(p, key, ksize);
	reqsize = 3 * 4 + ksize;

	srv = select_srv(db, key, ksize);
	t = srv_send(db, srv, buf, reqsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(db, srv, buf, bsize, &p, &psize);

	if (reply == REP_CACHE_MISS || reply == REP_NOTIN) {
		rv = 0;
		goto exit;
	} else if (reply == REP_ERR) {
		rv = -1;
		goto exit;
	} else if (reply != REP_OK && reply != REP_CACHE_HIT) {
		/* invalid response */
		rv = -1;
		goto exit;
	}

	/* we've got an answer (either REP_OK or REP_CACHE_HIT) */
	rv = * (uint32_t *) p;
	rv = ntohl(rv);
	if (rv > (psize - 4) || rv > vsize) {
		rv = 0;
		goto exit;
	}
	memcpy(val, p + 4, rv);

exit:
	free(buf);
	return rv;
}

ssize_t nmdb_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return do_get(db, key, ksize, val, vsize, 1);
}

ssize_t nmdb_cache_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return do_get(db, key, ksize, val, vsize, 0);
}



static int do_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize,
		int impact_db, int async)
{
	ssize_t rv, t;
	unsigned char *buf, *p;
	size_t bsize;
	uint32_t request, reply;
	struct nmdb_srv *srv;

	if (impact_db) {
		if (async)
			request = REQ_SET_ASYNC;
		else
			request = REQ_SET_SYNC;
	} else {
		request = REQ_CACHE_SET;
	}


	/* Use the same buffer for the request and the reply.
	 * Request: 4 bytes ver+id, 4 bytes request code, 4 bytes ksize, 4
	 *		bytes vsize, ksize bytes key, vsize bytes val.
	 * Reply: 4 bytes id, 4 bytes reply code.
	 */
	bsize = 4 + 4 + 4 + 4 + ksize + vsize;
	buf = malloc(bsize);
	if (buf == NULL)
		return -1;

	* (uint32_t *) buf = htonl( (PROTO_VER << 28) | ID_CODE );
	* ((uint32_t *) buf + 1) = htonl(request);
	* ((uint32_t *) buf + 2) = htonl(ksize);
	* ((uint32_t *) buf + 3) = htonl(vsize);
	p = buf + 4 * 4;
	memcpy(p, key, ksize);
	p += ksize;
	memcpy(p, val, vsize);

	srv = select_srv(db, key, ksize);
	t = srv_send(db, srv, buf, bsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(db, srv, buf, bsize, NULL, NULL);

	if (reply == REP_OK) {
		rv = 1;
		goto exit;
	}

	/* REP_ERR or invalid response */
	rv = -1;

exit:
	free(buf);
	return rv;

}

int nmdb_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return do_set(db, key, ksize, val, vsize, 1, 1);
}

int nmdb_set_sync(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return do_set(db, key, ksize, val, vsize, 1, 0);
}

int nmdb_cache_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return do_set(db, key, ksize, val, vsize, 0, 0);
}



int do_del(nmdb_t *db, const unsigned char *key, size_t ksize,
		int impact_db, int async)
{
	ssize_t rv, t;
	unsigned char *buf;
	size_t bsize;
	uint32_t request, reply;
	struct nmdb_srv *srv;

	if (impact_db) {
		if (async)
			request = REQ_DEL_ASYNC;
		else
			request = REQ_DEL_SYNC;
	} else {
		request = REQ_CACHE_DEL;
	}


	/* Use the same buffer for the request and the reply.
	 * Request: 4 bytes ver+id, 4 bytes request code, 4 bytes ksize,
	 * 		ksize bytes key.
	 * Reply: 4 bytes id, 4 bytes reply code.
	 */
	bsize = 8 + 4 + ksize;
	buf = malloc(bsize);
	if (buf == NULL)
		return -1;

	* (uint32_t *) buf = htonl( (PROTO_VER << 28) | ID_CODE );
	* ((uint32_t *) buf + 1) = htonl(request);
	* ((uint32_t *) buf + 2) = htonl(ksize);
	memcpy(buf + 3 * 4, key, ksize);

	srv = select_srv(db, key, ksize);
	t = srv_send(db, srv, buf, bsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(db, srv, buf, bsize, NULL, NULL);

	if (reply == REP_OK) {
		rv = 1;
		goto exit;
	} else if (reply == REP_NOTIN) {
		rv = 0;
		goto exit;
	}

	/* REP_ERR or invalid response */
	rv = -1;

exit:
	free(buf);
	return rv;

}

int nmdb_del(nmdb_t *db, const unsigned char *key, size_t ksize)
{
	return do_del(db, key, ksize, 1, 1);
}

int nmdb_del_sync(nmdb_t *db, const unsigned char *key, size_t ksize)
{
	return do_del(db, key, ksize, 1, 0);
}

int nmdb_cache_del(nmdb_t *db, const unsigned char *key, size_t ksize)
{
	return do_del(db, key, ksize, 0, 0);
}


