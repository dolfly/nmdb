
#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <stdio.h>		/* perror() */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <netinet/in.h>		/* INET stuff */
//#include <netinet/ip.h>		/* IP stuff */
#include <netinet/tcp.h>	/* TCP stuff */
#include <string.h>		/* memcpy() */
#include <unistd.h>		/* fcntl() */
#include <fcntl.h>		/* fcntl() */
#include <errno.h>		/* errno */

/* Workaround for libevent 1.1a: the header assumes u_char is typedef'ed to an
 * unsigned char, and that "struct timeval" is in scope. */
typedef unsigned char u_char;
#include <sys/time.h>
#include <event.h>		/* libevent stuff */

#include "tcp.h"
#include "common.h"
#include "queue.h"
#include "net-const.h"
#include "req.h"

#define printf(...) do { } while (0)

/* TCP socket structure. Used mainly to hold buffers from incomplete
 * recv()s. */
struct tcp_socket {
	int fd;
	struct sockaddr_in clisa;
	socklen_t clilen;
	struct event *evt;

	unsigned char *buf;
	size_t pktsize;
	size_t len;
	struct req_info *req;
};

static void tcp_recv(int fd, short event, void *arg);
static void parse_msg(struct tcp_socket *tcpsock, struct req_info *req,
		unsigned char *buf, size_t len);
static void parse_get(struct req_info *req, int impact_db);
static void parse_set(struct req_info *req, int impact_db, int async);
static void parse_del(struct req_info *req, int impact_db, int async);
static void parse_cas(struct req_info *req, int impact_db);

void tcp_reply_err(struct req_info *req, uint32_t reply);
void tcp_reply_get(struct req_info *req, uint32_t reply,
		unsigned char *val, size_t vsize);
void tcp_reply_set(struct req_info *req, uint32_t reply);
void tcp_reply_del(struct req_info *req, uint32_t reply);
void tcp_reply_cas(struct req_info *req, uint32_t reply);


/*
 * Miscelaneous helper functions
 */

static void tcp_socket_free(struct tcp_socket *tcpsock)
{
	if (tcpsock->evt)
		free(tcpsock->evt);
	if (tcpsock->buf)
		free(tcpsock->buf);
	if (tcpsock->req) {
		free(tcpsock->req);
	}
	free(tcpsock);
}

static void rep_send_error(const struct req_info *req, const unsigned int code)
{
	uint32_t l, r, c;
	unsigned char minibuf[4 * 4];

	/* Network format: length (4), ID (4), REP_ERR (4), error code (4) */
	l = htonl(4 + 4 + 4 + 4);
	r = htonl(REP_ERR);
	c = htonl(code);
	memcpy(minibuf, &l, 4);
	memcpy(minibuf + 4, &(req->id), 4);
	memcpy(minibuf + 8, &r, 4);
	memcpy(minibuf + 12, &c, 4);

	/* If this send fails, there's nothing to be done */
	r = send(req->fd, minibuf, 4 * 4, 0);

	if (r < 0) {
		perror("rep_send_error() failed");
	}
}


static int rep_send(const struct req_info *req, const unsigned char *buf,
		const size_t size)
{
	int rv;

	rv = send(req->fd, buf, size, 0);
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
	uint32_t len;
	unsigned char minibuf[12];

	len = htonl(12);
	reply = htonl(reply);
	memcpy(minibuf, &len, 4);
	memcpy(minibuf + 4, &(req->id), 4);
	memcpy(minibuf + 8, &reply, 4);
	rep_send(req, minibuf, 12);
	return;
}


/* Create a queue entry structure based on the parameters passed. Memory
 * allocated here will be free()'d in queue_entry_free(). It's not the
 * cleanest way, but the alternatives are even messier. */
static struct queue_entry *make_queue_long_entry(struct req_info *req,
		uint32_t operation, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize,
		const unsigned char *newval, size_t nvsize)
{
	struct queue_entry *e;
	unsigned char *kcopy, *vcopy, *nvcopy;

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

	nvcopy = NULL;
	if (newval != NULL) {
		nvcopy = malloc(nvsize);
		if (nvcopy == NULL) {
			queue_entry_free(e);
			if (kcopy != NULL)
				free(kcopy);
			if (vcopy != NULL)
				free(vcopy);
			return NULL;
		}
		memcpy(nvcopy, newval, nvsize);
	}

	e->operation = operation;
	e->key = kcopy;
	e->ksize = ksize;
	e->val = vcopy;
	e->vsize = vsize;
	e->newval = nvcopy;
	e->nvsize = nvsize;

	/* Create a copy of req, including clisa */
	e->req = malloc(sizeof(struct req_info));
	if (e->req == NULL) {
		queue_entry_free(e);
		return NULL;
	}
	memcpy(e->req, req, sizeof(struct req_info));

	e->req->clisa = malloc(sizeof(struct sockaddr_in));
	if (e->req->clisa == NULL) {
		queue_entry_free(e);
		return NULL;
	}
	memcpy(e->req->clisa, req->clisa, sizeof(struct sockaddr_in));

	/* clear out unused fields */
	e->req->payload = NULL;
	e->req->psize = 0;

	return e;
}

/* Like make_queue_long_entry() but with few parameters because most actions
 * do not need newval. */
static struct queue_entry *make_queue_entry(struct req_info *req,
		uint32_t operation, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return make_queue_long_entry(req, operation, key, ksize, val, vsize,
			NULL, 0);
}


void tcp_reply_err(struct req_info *req, uint32_t reply)
{
	rep_send_error(req, reply);
}

void tcp_reply_get(struct req_info *req, uint32_t reply,
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
		 * 4		total length
		 * 4		id
		 * 4		reply code
		 * 4		vsize
		 * vsize	val
		 */
		bsize = 4 + 4 + 4 + 4 + vsize;
		buf = malloc(bsize);

		t = htonl(bsize);
		memcpy(buf, &t, 4);

		memcpy(buf + 4, &(req->id), 4);
		memcpy(buf + 8, &reply, 4);

		t = htonl(vsize);
		memcpy(buf + 12, &t, 4);
		memcpy(buf + 16, val, vsize);

		rep_send(req, buf, bsize);
		free(buf);
	}
	return;

}


void tcp_reply_set(struct req_info *req, uint32_t reply)
{
	mini_reply(req, reply);
}


void tcp_reply_del(struct req_info *req, uint32_t reply)
{
	mini_reply(req, reply);
}

void tcp_reply_cas(struct req_info *req, uint32_t reply)
{
	mini_reply(req, reply);
}


/*
 * Main functions for receiving and parsing
 */

int tcp_init(void)
{
	int fd, rv;
	static struct sockaddr_in srvsa;


	srvsa.sin_family = AF_INET;
	srvsa.sin_addr.s_addr = INADDR_ANY;
	srvsa.sin_port = htons(20026);


	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	rv = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &rv, sizeof(rv)) < 0 ) {
		close(fd);
		return -1;
	}

	rv = bind(fd, (struct sockaddr *) &srvsa, sizeof(srvsa));
	if (rv < 0) {
		close(fd);
		return -1;
	}

	rv = listen(fd, 1024);
	if (rv < 0) {
		close(fd);
		return -1;
	}

	/* Disable nagle algorithm, as we often handle small amounts of data
	 * it can make I/O quite slow.
	 * XXX: back this up with real performance tests. */
	rv = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &rv, sizeof(rv)) < 0 ) {
		close(fd);
		return -1;
	}

	return fd;
}


void tcp_close(int fd)
{
	close(fd);
}


/* Called by libevent for each receive event on our listen fd */
void tcp_newconnection(int fd, short event, void *arg)
{
	int newfd, rv;
	struct tcp_socket *tcpsock;
	struct event *new_event;

	tcpsock = malloc(sizeof(struct tcp_socket));
	if (tcpsock == NULL) {
		return;
	}
	tcpsock->clilen = sizeof(tcpsock->clisa);

	new_event = malloc(sizeof(struct event));
	if (new_event == NULL) {
		free(tcpsock);
		return;
	}

	newfd = accept(fd,
			(struct sockaddr *) &(tcpsock->clisa),
			&(tcpsock->clilen));

	if (fcntl(newfd, F_SETFL, O_NONBLOCK) != 0) {
		close(newfd);
		free(new_event);
		free(tcpsock);
		return;
	}

	/* Disable nagle algorithm, as we often handle small amounts of data
	 * it can make I/O quite slow.
	 * XXX: back this up with real performance tests. */
	/* inherits from the listen fd? */
#if 0
	rv = 1;
	if (setsockopt(newfd, IPPROTO_TCP, TCP_NODELAY, &rv, sizeof(rv)) < 0 ) {
		close(newfd);
		return;
	}
#endif

	tcpsock->fd = newfd;
	tcpsock->evt = new_event;
	tcpsock->buf = NULL;
	tcpsock->pktsize = 0;
	tcpsock->len = 0;
	tcpsock->req = NULL;

	event_set(new_event, newfd, EV_READ | EV_PERSIST, tcp_recv,
			(void *) tcpsock);
	event_add(new_event, NULL);

	return;
}

/* Called by libevent for each receive event */
static void tcp_recv(int fd, short event, void *arg)
{
	int rv;
	size_t bsize;
	unsigned char *buf = NULL;
	struct tcp_socket *tcpsock;
	struct req_info *req = NULL;

	tcpsock = (struct tcp_socket *) arg;

	if (tcpsock->buf == NULL) {
		/* New incoming message */

		/* Allocate a little bit more over the max. message size,
		 * which is 64k; it will be freed by parse_msg(). */
		bsize = 68 * 1024;
		buf = malloc(bsize);
		if (buf == NULL) {
			goto error_exit;
		}

		rv = recv(fd, buf, bsize, 0);
		if (rv < 0 && errno == EAGAIN) {
			/* We were awoken but have no data to read, so we do
			 * nothing */
			return;
		} else if (rv <= 0) {
			/* Orderly shutdown or error; close the file
			 * descriptor in either case. */
			free(buf);
			goto error_exit;
		}

		/* parse_msg() will take care of freeing this when the time
		 * comes */
		req = malloc(sizeof(struct req_info));
		if (req == NULL) {
			free(req);
			goto error_exit;
		}

		req->fd = fd;
		req->type = REQTYPE_TCP;
		req->clisa = (struct sockaddr *) &tcpsock->clisa;
		req->clilen = tcpsock->clilen;
		req->reply_err = tcp_reply_err;
		req->reply_get = tcp_reply_get;
		req->reply_set = tcp_reply_set;
		req->reply_del = tcp_reply_del;
		req->reply_cas = tcp_reply_cas;

		parse_msg(tcpsock, req, buf, rv);

	} else {
		/* We already got a partial message, complete it. */
		size_t maxtoread = tcpsock->pktsize - tcpsock->len;
		printf("\t recv-> %tu - %tu = %tu\n", tcpsock->pktsize, tcpsock->len, maxtoread);

		rv = recv(fd, tcpsock->buf + tcpsock->len, maxtoread, 0);
		if (rv < 0 && errno == EAGAIN) {
			return;
		} else if (rv <= 0) {
			printf("err recv\n");
			goto error_exit;
		}

		tcpsock->len += rv;

		parse_msg(tcpsock, tcpsock->req, tcpsock->buf, tcpsock->len);
	}

	return;

error_exit:
	close(fd);
	event_del(tcpsock->evt);
	tcp_socket_free(tcpsock);
	return;
}


/* Main message parsing and dissecting */
static void parse_msg(struct tcp_socket *tcpsock, struct req_info *req,
		unsigned char *buf, size_t len)
{
	uint32_t hdr, id, cmd, totaltoget = 0;
	uint8_t ver;
	size_t psize;
	unsigned char *payload;

	printf("parse l:%tu tl:%tu tb:%p ts:%tu \n", len, tcpsock->len, tcpsock->buf, tcpsock->pktsize);

	if (len >= 4) {
		totaltoget = * (uint32_t *) buf;
		printf("got len: %u (%u)\n", ntohl(totaltoget), totaltoget);
		totaltoget = ntohl(totaltoget);
		if (totaltoget > (64 * 1024) || totaltoget <= 12) {
			/* Message too big or too small, close the connection. */
			printf("size err: %d\n", totaltoget);
			goto error_exit;
		}

	} else {
		/* If we didn't even read 4 bytes, we try to read 4 first and
		 * then care about the rest. */
		totaltoget = 4;
	}

	printf("totaltoget: %u vs %tu\n", totaltoget, len);

	if (totaltoget != len) {
		if (tcpsock->buf == NULL) {
			/* The first incomplete recv() */
			tcpsock->buf = buf;
			tcpsock->len = len;
			tcpsock->pktsize = totaltoget;
			tcpsock->req = req;

		} else {
			/* We already had an incomplete recv() and this is
			 * just another one. */
			tcpsock->len = len;
			tcpsock->pktsize = totaltoget;
		}

		return;
	}
	printf("parsing\n");

	/* The buffer is complete, parse it as usual. */

	/* The header is:
	 * 4 bytes	Total message length
	 * 4 bytes	Version + ID
	 * 4 bytes	Command
	 * Variable 	Payload
	 */

	hdr = * ((uint32_t *) buf + 1);
	hdr = htonl(hdr);

	/* FIXME: little endian-only */
	ver = (hdr & 0xF0000000) >> 28;
	id = hdr & 0x0FFFFFFF;
	req->id = id;

	cmd = ntohl(* ((uint32_t *) buf + 2));

	if (ver != PROTO_VER) {
		stats.net_version_mismatch++;
		rep_send_error(req, ERR_VER);
		goto exit;
	}

	/* We define payload as the stuff after buf. But be careful because
	 * there might be none (if len == 1). Doing the pointer arithmetic
	 * isn't problematic, but accessing the payload should be done only if
	 * we know we have enough data. */
	payload = buf + 12;
	psize = len - 12;

	/* Store the id encoded in network byte order, so that we don't have
	 * to calculate it at send time. */
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
	else if (cmd == REQ_SET_SYNC)
		parse_set(req, 1, 0);
	else if (cmd == REQ_DEL_SYNC)
		parse_del(req, 1, 0);
	else if (cmd == REQ_SET_ASYNC)
		parse_set(req, 1, 1);
	else if (cmd == REQ_DEL_ASYNC)
		parse_del(req, 1, 1);
	else if (cmd == REQ_CACHE_CAS)
		parse_cas(req, 0);
	else if (cmd == REQ_CAS)
		parse_cas(req, 1);
	else {
		stats.net_unk_req++;
		rep_send_error(req, ERR_UNKREQ);
	}

exit:
	printf("pm exit\n");
	/* We completed the read successfuly. buf and req were allocated by
	 * tcp_recv(), but they are freed here only after we have fully parsed
	 * the message. */
	if (tcpsock->buf) {
		tcpsock->buf = NULL;
		tcpsock->len = 0;
		tcpsock->pktsize = 0;
		tcpsock->req = NULL;
	}

	free(buf);
	free(req);

	return;


error_exit:
	printf("pm error\n");
		printf("t:%p b:%p\n", tcpsock->buf, buf);
	if (tcpsock->buf != buf) {
		free(tcpsock->buf);
	}
	free(buf);
	tcpsock->buf = NULL;

	/* We know that if tcpsock->req != NULL => tcpsock->req == req; so
	 * there is no need to do the conditional free(). */
	free(req);
	tcpsock->req = NULL;

	close(tcpsock->fd);
	event_del(tcpsock->evt);
	tcp_socket_free(tcpsock);
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
		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);
		queue_signal(op_queue);
		return;
	} else {
		tcp_reply_get(req, REP_CACHE_HIT, val, vsize);
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

		request = REQ_SET_SYNC;
		if (async)
			request = REQ_SET_ASYNC;

		e = make_queue_entry(req, request, key, ksize, val, vsize);
		if (e == NULL) {
			rep_send_error(req, ERR_MEM);
			return;
		}
		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);

		if (async) {
			mini_reply(req, REP_OK);
		} else {
			/* Signal the DB thread it has work only if it's a
			 * synchronous operation, asynchronous don't mind
			 * waiting. It does have a measurable impact on
			 * performance (2083847usec vs 2804973usec for sets on
			 * "test2d 100000 10 10"). */
			queue_signal(op_queue);
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

		request = REQ_DEL_SYNC;
		if (async)
			request = REQ_DEL_ASYNC;

		e = make_queue_entry(req, request, key, ksize, NULL, 0);
		if (e == NULL) {
			rep_send_error(req, ERR_MEM);
			return;
		}
		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);

		if (async) {
			mini_reply(req, REP_OK);
		} else {
			/* See comment on parse_set(). */
			queue_signal(op_queue);
		}

		return;
	}

	return;
}

static void parse_cas(struct req_info *req, int impact_db)
{
	int rv;
	unsigned char *key, *oldval, *newval;
	uint32_t ksize, ovsize, nvsize;
	const int max = 65536;

	/* Request format:
	 * 4		ksize
	 * 4		ovsize
	 * 4		nvsize
	 * ksize	key
	 * ovsize	oldval
	 * nvsize	newval
	 */
	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);
	ovsize = * ( ((uint32_t *) req->payload) + 1);
	ovsize = ntohl(ovsize);
	nvsize = * ( ((uint32_t *) req->payload) + 2);
	nvsize = ntohl(nvsize);

	/* Sanity check on sizes:
	 * - ksize, ovsize and nvsize must all be < req->psize
	 * - ksize, ovsize and nvsize must all be < 2^16 = 64k
	 * - ksize + ovsize + mvsize < 2^16 = 64k
	 */
	if ( (req->psize < ksize) || (req->psize < ovsize) ||
				(req->psize < nvsize) ||
			(ksize > max) || (ovsize > max) ||
				(nvsize > max) ||
			( (ksize + ovsize + nvsize) > max) ) {
		stats.net_broken_req++;
		rep_send_error(req, ERR_BROKEN);
		return;
	}

	key = req->payload + sizeof(uint32_t) * 3;
	oldval = key + ksize;
	newval = oldval + ovsize;

	rv = cache_cas(cache_table, key, ksize, oldval, ovsize,
			newval, nvsize);
	if (rv == 0) {
		/* If the cache doesn't match, there is no need to bother the
		 * DB even if we were asked to impact. */
		mini_reply(req, REP_NOMATCH);
		return;
	}

	if (!impact_db) {
		if (rv == -1) {
			mini_reply(req, REP_NOTIN);
			return;
		} else {
			mini_reply(req, REP_OK);
			return;
		}
	} else {
		/* impact_db = 1 and the key is either not in the cache, or
		 * cache_cas() was successful. We now need to queue the CAS in
		 * the database. */
		struct queue_entry *e;

		e = make_queue_long_entry(req, REQ_CAS, key, ksize,
				oldval, ovsize, newval, nvsize);
		if (e == NULL) {
			rep_send_error(req, ERR_MEM);
			return;
		}

		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);
		queue_signal(op_queue);
	}
	return;
}


