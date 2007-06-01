
#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <linux/tipc.h>		/* tipc stuff */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <string.h>		/* memcpy() */
#include <unistd.h>		/* close() */
#include <netinet/tcp.h>	/* TCP stuff */
#include <netdb.h>		/* gethostbyname() */

#include "nmdb.h"
#include "net-const.h"

/* The ID code for requests is hardcoded for now, until asynchronous requests
 * are implemented. */
#define ID_CODE 1

/* Different connection types. Used internally to differentiate between TIPC
 * and TCP connections in struct nmdb_srv. */
#define TIPC_CONN 1
#define TCP_CONN 2


/* Like recv() but either fails, or returns a complete read; if we return less
 * than count is because EOF was reached */
ssize_t srecv(int fd, unsigned char *buf, size_t count, int flags)
{
	ssize_t rv, c;

	c = 0;

	while (c < count) {
		rv = recv(fd, buf + c, count - c, flags);

		if (rv == count)
			return count;
		else if (rv < 0)
			return rv;
		else if (rv == 0)
			return c;

		c += rv;
	}
	return count;
}

/* Like srecv() but for send() */
ssize_t ssend(int fd, const unsigned char *buf, size_t count, int flags)
{
	ssize_t rv, c;

	c = 0;

	while (c < count) {
		rv = send(fd, buf + c, count - c, flags);

		if (rv == count)
			return count;
		else if (rv < 0)
			return rv;
		else if (rv == 0)
			return c;

		c += rv;
	}
	return count;
}



/* Create a nmdb_t and set the first server to port. If port is < 0, the
 * standard port is used. */
nmdb_t *nmdb_init()
{
	nmdb_t *db;

	db = malloc(sizeof(nmdb_t));
	if (db == NULL) {
		return NULL;
	}

	db->servers = NULL;
	db->nservers = 0;

	return db;
}

/* Compare two servers by their connection identifiers. It is used internally
 * to keep the server array sorted with qsort(). */
static int compare_servers(const void *s1, const void *s2)
{
	struct nmdb_srv *srv1 = (struct nmdb_srv *) s1;
	struct nmdb_srv *srv2 = (struct nmdb_srv *) s2;

	if (srv1->type != srv2->type) {
		if (srv1->type < srv2->type)
			return -1;
		else
			return 1;
	}

	if (srv1->type == TIPC_CONN) {
		if (srv1->info.tipc.port < srv2->info.tipc.port)
			return -1;
		else if (srv1->info.tipc.port == srv2->info.tipc.port)
			return 0;
		else
			return 1;
	} else if (srv1->type == TCP_CONN) {
		in_addr_t a1, a2;
		a1 = srv1->info.tcp.srvsa.sin_addr.s_addr;
		a2 = srv2->info.tcp.srvsa.sin_addr.s_addr;

		if (a1 < a2) {
			return -1;
		} else if (a1 == a2) {
			in_port_t p1, p2;
			p1 = srv1->info.tcp.srvsa.sin_port;
			p2 = srv2->info.tcp.srvsa.sin_port;

			if (p1 < p2)
				return -1;
			else if (p1 == p2)
				return 0;
			else
				return 1;
		} else {
			return 1;
		}
	}

	/* We should never get here */
	return 0;
}

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

	newsrv = &(db->servers[db->nservers - 1]);

	newsrv->fd = fd;

	if (port < 0)
		port = TIPC_SERVER_INST;

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

/* Used internally to really add the server once we have an IP address. */
static int add_tcp_server_addr(nmdb_t *db, in_addr_t *inetaddr, int port)
{
	int rv, fd;
	struct nmdb_srv *newsrv, *newarray;

	fd = socket(AF_INET, SOCK_STREAM, 0);
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
		port = TCP_SERVER_PORT;

	newsrv = &(db->servers[db->nservers - 1]);

	newsrv->fd = fd;
	newsrv->info.tcp.srvsa.sin_family = AF_INET;
	newsrv->info.tcp.srvsa.sin_port = htons(port);
	newsrv->info.tcp.srvsa.sin_addr.s_addr = *inetaddr;

	rv = connect(fd, (struct sockaddr *) &(newsrv->info.tcp.srvsa),
			sizeof(newsrv->info.tcp.srvsa));
	if (rv < 0)
		goto error_exit;

	/* Disable Nagle algorithm because we often send small packets. Huge
	 * gain in performance. */
	rv = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &rv, sizeof(rv)) < 0 )
		goto error_exit;

	newsrv->type = TCP_CONN;

	/* keep the list sorted by port, so we can do a reliable selection */
	qsort(db->servers, db->nservers, sizeof(struct nmdb_srv),
			compare_servers);

	return 1;

error_exit:
	close(fd);
	newarray = realloc(db->servers,
			sizeof(struct nmdb_srv) * (db->nservers - 1));
	if (newarray == NULL) {
		db->servers = NULL;
		db->nservers = 0;
		return 0;
	}

	db->servers = newarray;
	db->nservers -= 1;

	return 0;
}

/* Same as nmdb_add_tipc_server() but for TCP connections. */
int nmdb_add_tcp_server(nmdb_t *db, const char *addr, int port)
{
	int rv;
	struct hostent *he;
	struct in_addr ia;

	/* We try to resolve and then pass it to add_tcp_server_addr(). */
	rv = inet_pton(AF_INET, addr, &ia);
	if (rv <= 0) {
		he = gethostbyname(addr);
		if (he == NULL)
			return 0;

		ia.s_addr = *( (in_addr_t *) (he->h_addr_list[0]) );
	}

	return add_tcp_server_addr(db, &(ia.s_addr), port);
}

/* Frees a nmdb_t structure created with nmdb_init(). */
int nmdb_free(nmdb_t *db)
{
	if (db->servers != NULL) {
		int i;
		for (i = 0; i < db->nservers; i++)
			close(db->servers[i].fd);
		free(db->servers);
	}
	free(db);
	return 1;
}

static int tipc_srv_send(struct nmdb_srv *srv,
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

static int tcp_srv_send(struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize)
{
	ssize_t rv;
	uint32_t len;

	len = htonl(bsize + 4);
	rv = ssend(srv->fd, (unsigned char *) &len, 4, 0);
	if (rv != 4)
		return 0;

	rv = ssend(srv->fd, buf, bsize, 0);
	if (rv != bsize)
		return 0;
	return 1;
}

/* Used internally to send a buffer to the given server. Calls the appropriate
 * sender according to the server protocol. */
static int srv_send(struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize)
{
	if (srv == NULL)
		return 0;

	if (srv->type == TIPC_CONN)
		return tipc_srv_send(srv, buf, bsize);
	else if (srv->type == TCP_CONN)
		return tcp_srv_send(srv, buf, bsize);
	else
		return 0;
}


/* Used internally to get and parse replies from the server. */
static uint32_t tipc_get_rep(struct nmdb_srv *srv,
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

/* Used internally to get and parse replies from the server. */
static uint32_t tcp_get_rep(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize)
{
	ssize_t rv;
	uint32_t id, reply, msgsize;

	rv = srecv(srv->fd, (unsigned char *) &msgsize, 4, 0);
	if (rv != 4)
		return -1;

	msgsize = ntohl(msgsize);
	if (bsize < msgsize)
		return -1;

	rv = srecv(srv->fd, buf, msgsize - 4, 0);
	if (rv != msgsize - 4) {
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

static uint32_t get_rep(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize)
{
	if (srv == NULL)
		return -1;

	if (srv->type == TIPC_CONN)
		return tipc_get_rep(srv, buf, bsize, payload, psize);
	else if (srv->type == TCP_CONN)
		return tcp_get_rep(srv, buf, bsize, payload, psize);
	else
		return 0;
}


/* Hash function used internally by select_srv(). See RFC 1071. */
static uint32_t checksum(const unsigned char *buf, size_t bsize)
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

	if (db->nservers == 0)
		return NULL;

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
	 * 66000. We malloc 70k just in case.
	 */
	bsize = 70 * 1024;
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
	t = srv_send(srv, buf, reqsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(srv, buf, bsize, &p, &psize);

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
	t = srv_send(srv, buf, bsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(srv, buf, bsize, NULL, NULL);

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



static int do_del(nmdb_t *db, const unsigned char *key, size_t ksize,
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
	t = srv_send(srv, buf, bsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(srv, buf, bsize, NULL, NULL);

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


static int do_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize,
		int impact_db)
{
	ssize_t rv, t;
	unsigned char *buf, *p;
	size_t bsize;
	uint32_t request, reply;
	struct nmdb_srv *srv;

	request = REQ_CACHE_CAS;
	if (impact_db)
		request = REQ_CAS;


	/* Use the same buffer for the request and the reply.
	 * Request: 4 bytes ver+id, 4 bytes request code, 4 bytes ksize, 4
	 *		bytes ovsize, 4 bytes nvsize, ksize bytes key,
	 *		ovsize bytes oldval, nvsize bytes newval.
	 * Reply: 4 bytes id, 4 bytes reply code.
	 */
	bsize = 4 + 4 + 4 + 4 + 4 + ksize + ovsize + nvsize;
	buf = malloc(bsize);
	if (buf == NULL)
		return -1;

	* (uint32_t *) buf = htonl( (PROTO_VER << 28) | ID_CODE );
	* ((uint32_t *) buf + 1) = htonl(request);
	* ((uint32_t *) buf + 2) = htonl(ksize);
	* ((uint32_t *) buf + 3) = htonl(ovsize);
	* ((uint32_t *) buf + 4) = htonl(nvsize);
	p = buf + 5 * 4;
	memcpy(p, key, ksize);
	p += ksize;
	memcpy(p, oldval, ovsize);
	p += ovsize;
	memcpy(p, newval, nvsize);

	srv = select_srv(db, key, ksize);
	t = srv_send(srv, buf, bsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(srv, buf, bsize, NULL, NULL);

	if (reply == REP_OK) {
		rv = 2;
		goto exit;
	} else if (reply == REP_NOMATCH) {
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

int nmdb_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize)
{
	return do_cas(db, key, ksize, oldval, ovsize, newval, nvsize, 1);
}

int nmdb_cache_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize)
{
	return do_cas(db, key, ksize, oldval, ovsize, newval, nvsize, 0);
}

