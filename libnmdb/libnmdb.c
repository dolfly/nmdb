
#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <string.h>		/* memcpy() */
#include <unistd.h>		/* close() */

#include "nmdb.h"
#include "internal.h"
#include "net-const.h"
#include "tipc.h"
#include "tcp.h"
#include "udp.h"
#include "sctp.h"
#include "netutils.h"


/* Possible flags, notice it may make no sense to mix them, consult the
 * documentation before doing weird things. Values should be kept in sync with
 * the internal net-const.h */
#define NMDB_CACHE_ONLY 1
#define NMDB_SYNC 2


/* Compares two servers by their connection identifiers. It is used internally
 * to keep the server array sorted with qsort(). */
int compare_servers(const void *s1, const void *s2)
{
	struct nmdb_srv *srv1 = (struct nmdb_srv *) s1;
	struct nmdb_srv *srv2 = (struct nmdb_srv *) s2;

	if (srv1->type != srv2->type) {
		if (srv1->type < srv2->type)
			return -1;
		else
			return 1;
	}

#if ENABLE_TIPC
	if (srv1->type == TIPC_CONN) {
		if (srv1->info.tipc.port < srv2->info.tipc.port)
			return -1;
		else if (srv1->info.tipc.port == srv2->info.tipc.port)
			return 0;
		else
			return 1;
	}
#endif
#if ENABLE_TCP || ENABLE_UDP || ENABLE_SCTP
	if (srv1->type == TCP_CONN
			|| srv1->type == UDP_CONN
			|| srv1->type == SCTP_CONN) {
		in_addr_t a1, a2;
		a1 = srv1->info.in.srvsa.sin_addr.s_addr;
		a2 = srv2->info.in.srvsa.sin_addr.s_addr;

		if (a1 < a2) {
			return -1;
		} else if (a1 == a2) {
			in_port_t p1, p2;
			p1 = srv1->info.in.srvsa.sin_port;
			p2 = srv2->info.in.srvsa.sin_port;

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
#endif

	/* We should never get here */
	return 0;
}


/* Like recv(), but either fails, or returns a complete read; if we return
 * less than count is because EOF was reached. */
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

/* Like srecv(), but for send(). */
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

/* Creates a nmdb_t. */
nmdb_t *nmdb_init(void)
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

/* Sends a buffer to the given server. */
static int srv_send(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize)
{
	if (srv == NULL)
		return 0;

	switch (srv->type) {
		case TIPC_CONN:
			return tipc_srv_send(srv, buf, bsize);
		case TCP_CONN:
			return tcp_srv_send(srv, buf, bsize);
		case UDP_CONN:
			return udp_srv_send(srv, buf, bsize);
		case SCTP_CONN:
			return sctp_srv_send(srv, buf, bsize);
		default:
			return 0;
	}
}

/* Gets a reply from the given server. */
static uint32_t get_rep(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize)
{
	if (srv == NULL)
		return -1;

	switch (srv->type) {
		case TIPC_CONN:
			return tipc_get_rep(srv, buf, bsize, payload, psize);
		case TCP_CONN:
			return tcp_get_rep(srv, buf, bsize, payload, psize);
		case UDP_CONN:
			return udp_get_rep(srv, buf, bsize, payload, psize);
		case SCTP_CONN:
			return sctp_get_rep(srv, buf, bsize, payload, psize);
		default:
			return 0;
	}
}

/* When a packet arrives, the message it contains begins on a
 * protocol-dependant offset. This functions returns the offset to use when
 * sending/receiving messages for the given server. */
static int srv_get_msg_offset(struct nmdb_srv *srv)
{
	if (srv == NULL)
		return 0;

	switch (srv->type) {
		case TIPC_CONN:
			return TIPC_MSG_OFFSET;
		case TCP_CONN:
			return TCP_MSG_OFFSET;
		case UDP_CONN:
			return UDP_MSG_OFFSET;
		case SCTP_CONN:
			return SCTP_MSG_OFFSET;
		default:
			return 0;
	}
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

/* Selects which server to use for the given key. */
static struct nmdb_srv *select_srv(nmdb_t *db,
		const unsigned char *key, size_t ksize)
{
	uint32_t n;

	if (db->nservers == 0)
		return NULL;

	n = checksum(key, ksize) % db->nservers;
	return &(db->servers[n]);
}

/* Creates a new buffer for packets. */
static unsigned char *new_packet(struct nmdb_srv *srv, unsigned int request,
		unsigned short flags, size_t *bufsize, size_t *payload_offset,
		ssize_t payload_size)
{
	unsigned char *buf, *p;
	unsigned int moff = srv_get_msg_offset(srv);

	if (payload_size == -1) {
		/* Because our callers will reuse the buffer to get the reply,
		 * and we don't know how big it will be, we just alloc a bit
		 * over the max packet (64kb) */
		*bufsize = 68 * 1024;
	} else {
		/* 8 is the size of the common header */
		*bufsize = moff + 8 + payload_size;
	}
	buf = malloc(*bufsize);
	if (buf == NULL)
		return NULL;

	p = buf + moff;

	* (uint32_t *) p = htonl( (PROTO_VER << 28) | ID_CODE );
	* ((uint16_t *) p + 2) = htons(request);
	* ((uint16_t *) p + 3) = htons(flags);

	if (payload_offset != NULL)
		*payload_offset = moff + 8;

	return buf;
}

/* Functions to append different numbers of (value, len) to the given buffer;
 * it's not worth the trouble of making this generic because we never go past
 * three and they're quite trivial. */
static size_t append_1v(unsigned char *buf,
		const unsigned char *datum, size_t dsize)
{
	* ((uint32_t *) buf) = htonl(dsize);
	memcpy(buf + 4, datum, dsize);
	return 4 + dsize;
}

static size_t append_2v(unsigned char *buf,
		const unsigned char *datum1, size_t dsize1,
		const unsigned char *datum2, size_t dsize2)
{
	* ((uint32_t *) buf) = htonl(dsize1);
	* ((uint32_t *) buf + 1) = htonl(dsize2);
	memcpy(buf + 8, datum1, dsize1);
	memcpy(buf + 8 + dsize1, datum2, dsize2);
	return 8 + dsize1 + dsize2;
}

static size_t append_3v(unsigned char *buf,
		const unsigned char *datum1, size_t dsize1,
		const unsigned char *datum2, size_t dsize2,
		const unsigned char *datum3, size_t dsize3)
{
	* ((uint32_t *) buf) = htonl(dsize1);
	* ((uint32_t *) buf + 1) = htonl(dsize2);
	* ((uint32_t *) buf + 2) = htonl(dsize3);
	memcpy(buf + 12, datum1, dsize1);
	memcpy(buf + 12 + dsize1, datum2, dsize2);
	memcpy(buf + 12 + dsize1 + dsize2, datum3, dsize3);
	return 12 + dsize1 + dsize2 + dsize3;
}


/* Functions to perform a get. */
static ssize_t do_get(nmdb_t *db,
		const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize, unsigned short flags)
{
	ssize_t rv, t;
	unsigned char *buf, *p;
	size_t bufsize, reqsize, payload_offset, psize = 0;
	uint32_t reply;
	struct nmdb_srv *srv;

	flags = flags & NMDB_CACHE_ONLY;

	srv = select_srv(db, key, ksize);
	buf = new_packet(srv, REQ_GET, flags, &bufsize, &payload_offset, -1);
	if (buf == NULL)
		return -1;
	reqsize = payload_offset;
	reqsize += append_1v(buf + payload_offset, key, ksize);

	t = srv_send(srv, buf, reqsize);
	if (t <= 0) {
		rv = -2;
		goto exit;
	}

	reply = get_rep(srv, buf, bufsize, &p, &psize);

	if (reply == REP_CACHE_MISS || reply == REP_NOTIN) {
		rv = -1;
		goto exit;
	} else if (reply == REP_ERR) {
		rv = -2;
		goto exit;
	} else if (reply != REP_OK && reply != REP_CACHE_HIT) {
		/* invalid response */
		rv = -2;
		goto exit;
	}

	/* we've got an answer (either REP_OK or REP_CACHE_HIT) */
	rv = * (uint32_t *) p;
	rv = ntohl(rv);
	if (rv > (psize - 4) || rv > vsize) {
		/* the value is too big for the packet size, or it is too big
		 * to fit in the buffer we were given */
		rv = -2;
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
	return do_get(db, key, ksize, val, vsize, 0);
}

ssize_t nmdb_cache_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return do_get(db, key, ksize, val, vsize, NMDB_CACHE_ONLY);
}


/* Functions to perform a set. */
static int do_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize,
		unsigned short flags)
{
	ssize_t rv, t;
	unsigned char *buf;
	size_t bufsize, payload_offset, reqsize;
	uint32_t reply;
	struct nmdb_srv *srv;

	flags = flags & (NMDB_CACHE_ONLY | NMDB_SYNC);

	srv = select_srv(db, key, ksize);

	buf = new_packet(srv, REQ_SET, flags, &bufsize, &payload_offset,
			4 * 2 + ksize + vsize);
	if (buf == NULL)
		return -1;
	reqsize = payload_offset;
	reqsize += append_2v(buf + payload_offset, key, ksize, val, vsize);

	t = srv_send(srv, buf, reqsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(srv, buf, bufsize, NULL, NULL);

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
	return do_set(db, key, ksize, val, vsize, 0);
}

int nmdb_set_sync(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return do_set(db, key, ksize, val, vsize, NMDB_SYNC);
}

int nmdb_cache_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return do_set(db, key, ksize, val, vsize, NMDB_CACHE_ONLY);
}


/* Functions to perform a del. */
static int do_del(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned short flags)
{
	ssize_t rv, t;
	unsigned char *buf;
	size_t bufsize, payload_offset, reqsize;
	uint32_t reply;
	struct nmdb_srv *srv;

	flags = flags & (NMDB_CACHE_ONLY | NMDB_SYNC);

	srv = select_srv(db, key, ksize);

	buf = new_packet(srv, REQ_DEL, flags, &bufsize, &payload_offset,
			4 + ksize);
	if (buf == NULL)
		return -1;
	reqsize = payload_offset;
	reqsize += append_1v(buf + payload_offset, key, ksize);

	t = srv_send(srv, buf, reqsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(srv, buf, bufsize, NULL, NULL);

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
	return do_del(db, key, ksize, 0);
}

int nmdb_del_sync(nmdb_t *db, const unsigned char *key, size_t ksize)
{
	return do_del(db, key, ksize, NMDB_SYNC);
}

int nmdb_cache_del(nmdb_t *db, const unsigned char *key, size_t ksize)
{
	return do_del(db, key, ksize, NMDB_CACHE_ONLY);
}


/* Functions to perform a CAS. */
static int do_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize,
		unsigned short flags)
{
	ssize_t rv, t;
	unsigned char *buf;
	size_t bufsize, payload_offset, reqsize;
	uint32_t reply;
	struct nmdb_srv *srv;

	flags = flags & NMDB_CACHE_ONLY;

	srv = select_srv(db, key, ksize);

	buf = new_packet(srv, REQ_CAS, flags, &bufsize, &payload_offset,
			4 * 3 + ksize + ovsize + nvsize);
	if (buf == NULL)
		return -1;
	reqsize = payload_offset;
	reqsize += append_3v(buf + payload_offset, key, ksize, oldval, ovsize,
			newval, nvsize);

	t = srv_send(srv, buf, reqsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	reply = get_rep(srv, buf, bufsize, NULL, NULL);

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
	return do_cas(db, key, ksize, oldval, ovsize, newval, nvsize, 0);
}

int nmdb_cache_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize)
{
	return do_cas(db, key, ksize, oldval, ovsize, newval, nvsize,
			NMDB_CACHE_ONLY);
}


/* Functions to perform an atomic increment. */
static int do_incr(nmdb_t *db, const unsigned char *key, size_t ksize,
		int64_t increment, int64_t *newval, unsigned short flags)
{
	ssize_t rv, t;
	unsigned char *buf, *payload;
	size_t bufsize, payload_offset, reqsize, psize;
	uint32_t reply;
	struct nmdb_srv *srv;

	flags = flags & NMDB_CACHE_ONLY;

	srv = select_srv(db, key, ksize);

	increment = htonll(increment);

	buf = new_packet(srv, REQ_INCR, flags, &bufsize, &payload_offset,
			4 + ksize + sizeof(int64_t));
	if (buf == NULL)
		return -1;
	reqsize = payload_offset;
	reqsize += append_1v(buf + payload_offset, key, ksize);
	memcpy(buf + reqsize, &increment, sizeof(int64_t));
	reqsize += sizeof(int64_t);

	t = srv_send(srv, buf, reqsize);
	if (t <= 0) {
		rv = -1;
		goto exit;
	}

	psize = sizeof(int64_t);
	reply = get_rep(srv, buf, bufsize, &payload, &psize);

	switch (reply) {
		case REP_OK:
			rv = 2;
			if (newval != NULL && psize == sizeof(int64_t) + 4) {
				/* skip the 4 bytes of length */
				*newval = *((int64_t *) (payload + 4));
				*newval = ntohll(*newval);
			}
			break;
		case REP_NOMATCH:
			rv = 1;
			break;
		case REP_NOTIN:
			rv = 0;
			break;
		default:
			rv = -1;
			break;
	}

exit:
	free(buf);
	return rv;

}

int nmdb_incr(nmdb_t *db, const unsigned char *key, size_t ksize,
		int64_t increment, int64_t *newval)
{
	return do_incr(db, key, ksize, increment, newval, 0);
}

int nmdb_cache_incr(nmdb_t *db, const unsigned char *key, size_t ksize,
		int64_t increment, int64_t *newval)
{
	return do_incr(db, key, ksize, increment, newval, NMDB_CACHE_ONLY);
}


/* Request servers' statistics, return the aggregated results in buf, with the
 * number of servers in nservers and the number of stats per server in nstats.
 * Used in the "nmdb-stats" utility, matches the server version.
 *
 * Return:
 *   1 if success
 *  -1 if there was an error in the server
 *  -2 if there was a network error
 *  -3 if the buffer was too small
 *  -4 if the server replies were of different size (indicates different
 *     server versions, not supported at the time)
 *
 * TODO: The API could be improved by having nstats be provided by the caller,
 * and making sure its used by all servers. Also buf should be an uint64_t
 * *buf to make the typing more explicit. */
int nmdb_stats(nmdb_t *db, unsigned char *buf, size_t bsize,
		unsigned int *nservers, unsigned int *nstats)
{
	int i;
	size_t reqsize;
	ssize_t t, reply;
	unsigned char *request;
	struct nmdb_srv *srv;

	/* This buffer is used for a single reply, must be big enough to
	 * hold STATS_REPLY_SIZE. 32 elements is enough to allow future
	 * improvements. */
	unsigned char tmpbuf[32 * sizeof(uint64_t)];
	size_t tmpbufsize = 32 * sizeof(uint64_t);

	unsigned char *payload;
	size_t psize;

	*nstats = 0;

	for (i = 0; i < db->nservers; i++) {
		srv = db->servers + i;
		request = new_packet(srv, REQ_STATS, 0, &reqsize, NULL, 0);

		t = srv_send(srv, request, reqsize);
		free(request);
		if (t <= 0)
			return -2;

		reply = get_rep(srv, tmpbuf, tmpbufsize, &payload, &psize);
		if (reply != REP_OK)
			return -1;

		/* Check if there is enough room for the copy */
		if (bsize < psize)
			return -3;

		/* Now copy the reply in the buffer given to us, skipping the
		 * 4 bytes of length */
		memcpy(buf, payload + 4, psize);
		buf += psize;
		bsize -= psize;

		/* nstats should be the same for all the servers; if not,
		 * return error because the caller can't cope with that
		 * situation */
		if (*nstats == 0) {
			*nstats = psize / sizeof(uint64_t);
		} else if (*nstats != psize / sizeof(uint64_t)) {
			return -4;
		}
	}

	*nservers = db->nservers;

	return 1;
}


