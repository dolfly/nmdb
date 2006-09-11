
#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <linux/tipc.h>		/* tipc stuff */
#include <stdio.h>		/* perror() */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <string.h>		/* memcpy() */

#include "tipc.h"
#include "common.h"
#include "queue.h"
#include "net-const.h"


static void parse_msg(struct req_info *req, unsigned char *buf,
		size_t bsize);
static void parse_get(struct req_info *req, int impact_db);
static void parse_set(struct req_info *req, int impact_db, int async);
static void parse_del(struct req_info *req, int impact_db, int async);


/*
 * Miscelaneous helper functions
 */

static void rep_send_error(const struct req_info *req, const unsigned int code)
{
	int r, c;
	unsigned char minibuf[3 * 4];

	/* Network format: ID (4), REP_ERR (4), error code (4) */
	r = htonl(REP_ERR);
	c = htonl(code);
	memcpy(minibuf, &(req->id), 4);
	memcpy(minibuf + 4, &r, 4);
	memcpy(minibuf + 8, &c, 4);

	/* If this send fails, there's nothing to be done */
	r = sendto(req->fd, minibuf, 3 * 4, 0, (struct sockaddr *) req->clisa,
			req->clilen);

	if (r < 0) {
		perror("rep_send_error() failed");
	}
}


static int rep_send(const struct req_info *req, const unsigned char *buf,
		const size_t size)
{
	int rv;
	rv = sendto(req->fd, buf, size, 0,
			(struct sockaddr *) req->clisa, req->clilen);
	if (rv < 0) {
		rep_send_error(req, ERR_SEND);
		return 0;
	}
	return 1;
}


/* Send small replies, consisting in only a value. */
static void mini_reply(struct req_info *req, uint32_t reply)
{
	/* We use a mini buffer to speedup the small replies, to avoid the
	 * malloc() overhead. */
	unsigned char minibuf[8];

	reply = htonl(reply);
	memcpy(minibuf, &(req->id), 4);
	memcpy(minibuf + 4, &reply, 4);
	rep_send(req, minibuf, 8);
	return;
}


/* Create a queue entry structure based on the parameters passed. Memory
 * allocated here will be free()'d in queue_entry_free(). It's not the
 * cleanest way, but the alternatives are even messier. */
static struct queue_entry *make_queue_entry(struct req_info *req,
		uint32_t operation, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	struct queue_entry *e;
	unsigned char *kcopy, *vcopy;

	e = queue_entry_create();
	if (e == NULL) {
		return NULL;
	}

	kcopy = NULL;
	if (key != NULL) {
		kcopy = malloc(ksize);
		if (kcopy == NULL) {
			queue_entry_free(e);
			return NULL;
		}
		memcpy(kcopy, key, ksize);
	}

	vcopy = NULL;
	if (val != NULL) {
		vcopy = malloc(vsize);
		if (vcopy == NULL) {
			queue_entry_free(e);
			if (kcopy != NULL)
				free(kcopy);
			return NULL;
		}
		memcpy(vcopy, val, vsize);
	}

	e->operation = operation;
	e->key = kcopy;
	e->ksize = ksize;
	e->val = vcopy;
	e->vsize = vsize;

	/* Create a copy of req, including clisa */
	e->req = malloc(sizeof(struct req_info));
	if (e->req == NULL) {
		queue_entry_free(e);
		return NULL;
	}
	memcpy(e->req, req, sizeof(struct req_info));

	e->req->clisa = malloc(sizeof(struct sockaddr_tipc));
	if (e->req->clisa == NULL) {
		queue_entry_free(e);
		return NULL;
	}
	memcpy(e->req->clisa, req->clisa, sizeof(struct sockaddr_tipc));

	/* clear out unused fields */
	e->req->payload = NULL;
	e->req->psize = 0;

	return e;
}


/* The tipc_reply_* functions are used by the db code to send the network
 * replies. */

void tipc_reply_err(struct req_info *req, uint32_t reply)
{
	rep_send_error(req, reply);
}

void tipc_reply_get(struct req_info *req, uint32_t reply,
			unsigned char *val, size_t vsize)
{
	if (val == NULL) {
		/* miss */
		mini_reply(req, reply);
	} else {
		unsigned char *buf;
		size_t bsize;
		uint32_t t;

		reply = htonl(reply);

		/* The reply length is:
		 * 4		id
		 * 4		reply code
		 * 4		vsize
		 * vsize	val
		 */
		bsize = 4 + 4 + 4 + vsize;
		buf = malloc(bsize);

		t = htonl(vsize);

		memcpy(buf, &(req->id), 4);
		memcpy(buf + 4, &reply, 4);
		memcpy(buf + 8, &t, 4);
		memcpy(buf + 12, val, vsize);

		rep_send(req, buf, bsize);
		free(buf);
	}
	return;

}


void tipc_reply_set(struct req_info *req, uint32_t reply)
{
	mini_reply(req, reply);
}


void tipc_reply_del(struct req_info *req, uint32_t reply)
{
	mini_reply(req, reply);
}


/*
 * Main functions for receiving and parsing
 */

int tipc_init(void)
{
	int fd, rv;
	static struct sockaddr_tipc srvsa;

	srvsa.family = AF_TIPC;
	srvsa.addrtype = TIPC_ADDR_NAMESEQ;
	srvsa.addr.nameseq.type = SERVER_TYPE;
	srvsa.addr.nameseq.lower = SERVER_INST;
	srvsa.addr.nameseq.upper = SERVER_INST;
	srvsa.scope = TIPC_CLUSTER_SCOPE;

	fd = socket(AF_TIPC, SOCK_RDM, 0);
	if (fd < 0)
		return -1;

	rv = bind(fd, (struct sockaddr *) &srvsa, sizeof(srvsa));
	if (rv < 0)
		return -1;

	return fd;
}

/* Called by libevent for each receive event */
void tipc_recv(int fd, short event, void *arg)
{
	int rv;
	struct req_info req;
	struct sockaddr_tipc clisa;
	socklen_t clilen;
	size_t bsize;

	/* Allocate enough to hold the max msg length of 66000 bytes.
	 * Originally, this was malloc()ed, but using the stack made it go
	 * from 27 usec for each set operation, to 23 usec. While it may sound
	 * worthless, it made test1 go from 3.213s to 2.345s for 37618
	 * operations.
	 * TODO: check for negative impacts (beside being ugly, obviously)
	 */
	unsigned char buf[128 * 1024];
	bsize = 128 * 1024;

	clilen = sizeof(clisa);

	rv = recvfrom(fd, buf, bsize, 0, (struct sockaddr *) &clisa,
			&clilen);
	if (rv <= 0) {
		/* rv == 0 means "return of an undeliverable message", which
		 * we ignore; -1 means other error. */
		goto exit;
	}

	if (rv < 2) {
		stats.net_broken_req++;
		goto exit;
	}

	req.fd = fd;
	req.clisa = &clisa;
	req.clilen = clilen;

	/* parse the message */
	parse_msg(&req, buf, rv);

exit:
	return;
}


/* Main message parsing and dissecting */
static void parse_msg(struct req_info *req, unsigned char *buf, size_t bsize)
{
	uint32_t hdr, id, cmd;
	uint8_t ver;
	size_t psize;
	unsigned char *payload;

	hdr = * (uint32_t *) buf;
	hdr = htonl(hdr);

	/* FIXME: little endian-only */
	ver = (hdr & 0xF0000000) >> 28;
	id = hdr & 0x0FFFFFFF;

	cmd = ntohl(* ((uint32_t *) buf + 1));

	if (ver != PROTO_VER) {
		stats.net_version_mismatch++;
		rep_send_error(req, ERR_VER);
		return;
	}

	/* We define payload as the stuff after buf. But be careful because
	 * there might be none (if bsize == 1). Doing the pointer arithmetic
	 * isn't problematic, but accessing the payload should be done only if
	 * we know we have enough data. */
	payload = buf + 8;
	psize = bsize - 8;

	/* store the id encoded in network byte order, so that we don't have
	 * to calculate it at send time */
	req->id = htonl(id);
	req->cmd = cmd;
	req->payload = payload;
	req->psize = psize;

	if (cmd == REQ_CACHE_GET)
		parse_get(req, 0);
	else if (cmd == REQ_CACHE_SET)
		parse_set(req, 0, 0);
	else if (cmd == REQ_CACHE_DEL)
		parse_del(req, 0, 0);
	else if (cmd == REQ_GET)
		parse_get(req, 1);
	else if (cmd == REQ_SET)
		parse_set(req, 1, 0);
	else if (cmd == REQ_DEL)
		parse_del(req, 1, 0);
	else if (cmd == REQ_SET_ASYNC)
		parse_set(req, 1, 1);
	else if (cmd == REQ_DEL_ASYNC)
		parse_del(req, 1, 1);
	else {
		stats.net_unk_req++;
		rep_send_error(req, ERR_UNKREQ);
		return;
	}

	return;
}


static void parse_get(struct req_info *req, int impact_db)
{
	int hit;
	unsigned char *key;
	uint32_t ksize;
	unsigned char *val = NULL;
	size_t vsize = 0;

	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);
	if (req->psize < ksize) {
		stats.net_broken_req++;
		rep_send_error(req, ERR_BROKEN);
		return;
	}

	key = req->payload + sizeof(uint32_t);

	hit = cache_get(cache_table, key, ksize, &val, &vsize);

	if (!hit && !impact_db) {
		mini_reply(req, REP_CACHE_MISS);
		return;
	} else if (!hit && impact_db) {
		struct queue_entry *e;
		e = make_queue_entry(req, REQ_GET, key, ksize, NULL, 0);
		if (e == NULL) {
			rep_send_error(req, ERR_MEM);
			return;
		}
		queue_put(op_queue, e);
		return;
	} else {
		tipc_reply_get(req, REP_CACHE_HIT, val, vsize);
		return;
	}
}


static void parse_set(struct req_info *req, int impact_db, int async)
{
	int rv;
	unsigned char *key, *val;
	uint32_t ksize, vsize;
	const int max = 65536;

	/* Request format:
	 * 4		ksize
	 * 4		vsize
	 * ksize	key
	 * vsize	val
	 */
	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);
	vsize = * ( ((uint32_t *) req->payload) + 1),
	vsize = ntohl(vsize);

	/* Sanity check on sizes:
	 * - ksize and vsize must both be < req->psize
	 * - ksize and vsize must both be < 2^16 = 64k
	 * - ksize + vsize < 2^16 = 64k
	 */
	if ( (req->psize < ksize) || (req->psize < vsize) ||
			(ksize > max) || (vsize > max) ||
			( (ksize + vsize) > max) ) {
		stats.net_broken_req++;
		rep_send_error(req, ERR_BROKEN);
		return;
	}

	key = req->payload + sizeof(uint32_t) * 2;
	val = key + ksize;

	rv = cache_set(cache_table, key, ksize, val, vsize);
	if (!rv) {
		rep_send_error(req, ERR_MEM);
		return;
	}

	if (impact_db) {
		struct queue_entry *e;
		uint32_t request;

		request = REQ_SET;
		if (async)
			request = REQ_SET_ASYNC;

		e = make_queue_entry(req, request, key, ksize, val, vsize);
		if (e == NULL) {
			rep_send_error(req, ERR_MEM);
			return;
		}
		queue_put(op_queue, e);

		if (async) {
			mini_reply(req, REP_OK);
		}
		return;
	} else {
		mini_reply(req, REP_OK);
	}

	return;
}


static void parse_del(struct req_info *req, int impact_db, int async)
{
	int hit;
	unsigned char *key;
	uint32_t ksize;

	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);
	if (req->psize < ksize) {
		stats.net_broken_req++;
		rep_send_error(req, ERR_BROKEN);
		return;
	}

	key = req->payload + sizeof(uint32_t);

	hit = cache_del(cache_table, key, ksize);

	if (!impact_db && hit) {
		mini_reply(req, REP_OK);
	} else if (!impact_db && !hit) {
		mini_reply(req, REP_NOTIN);
	} else if (impact_db) {
		struct queue_entry *e;
		uint32_t request;

		request = REQ_DEL;
		if (async)
			request = REQ_DEL_ASYNC;

		e = make_queue_entry(req, request, key, ksize, NULL, 0);
		if (e == NULL) {
			rep_send_error(req, ERR_MEM);
			return;
		}
		queue_put(op_queue, e);

		if (async) {
			mini_reply(req, REP_OK);
		}
		return;
	}

	return;
}


