
#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <stdio.h>		/* perror() */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <netinet/in.h>		/* INET stuff */
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
#include "net-const.h"
#include "req.h"
#include "parse.h"


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
	size_t excess;
};

static void tcp_recv(int fd, short event, void *arg);
static void process_buf(struct tcp_socket *tcpsock, struct req_info *req,
		unsigned char *buf, size_t len);

static void tcp_mini_reply(struct req_info *req, uint32_t reply);
static void tcp_reply_err(struct req_info *req, uint32_t reply);
static void tcp_reply_get(struct req_info *req, uint32_t reply,
		unsigned char *val, size_t vsize);
static void tcp_reply_set(struct req_info *req, uint32_t reply);
static void tcp_reply_del(struct req_info *req, uint32_t reply);
static void tcp_reply_cas(struct req_info *req, uint32_t reply);


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

struct req_info *build_req(struct tcp_socket *tcpsock)
{
	struct req_info *req;

	/* Our caller will take care of freeing this when the time comes */
	req = malloc(sizeof(struct req_info));
	if (req == NULL)
		return NULL;

	req->fd = tcpsock->fd;
	req->type = REQTYPE_TCP;
	req->clisa = (struct sockaddr *) &tcpsock->clisa;
	req->clilen = tcpsock->clilen;
	req->mini_reply = tcp_mini_reply;
	req->reply_err = tcp_reply_err;
	req->reply_get = tcp_reply_get;
	req->reply_set = tcp_reply_set;
	req->reply_del = tcp_reply_del;
	req->reply_cas = tcp_reply_cas;

	return req;
}

static void rep_send_error(const struct req_info *req, const unsigned int code)
{
	uint32_t l, r, c;
	unsigned char minibuf[4 * 4];

	if (settings.passive)
		return;

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

	if (settings.passive)
		return 1;

	rv = send(req->fd, buf, size, 0);
	if (rv < 0) {
		rep_send_error(req, ERR_SEND);
		return 0;
	}
	return 1;
}


/* Send small replies, consisting in only a value. */
void tcp_mini_reply(struct req_info *req, uint32_t reply)
{
	/* We use a mini buffer to speedup the small replies, to avoid the
	 * malloc() overhead. */
	uint32_t len;
	unsigned char minibuf[12];

	if (settings.passive)
		return;

	len = htonl(12);
	reply = htonl(reply);
	memcpy(minibuf, &len, 4);
	memcpy(minibuf + 4, &(req->id), 4);
	memcpy(minibuf + 8, &reply, 4);
	rep_send(req, minibuf, 12);
	return;
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
		tcp_mini_reply(req, reply);
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
	tcp_mini_reply(req, reply);
}


void tcp_reply_del(struct req_info *req, uint32_t reply)
{
	tcp_mini_reply(req, reply);
}

void tcp_reply_cas(struct req_info *req, uint32_t reply)
{
	tcp_mini_reply(req, reply);
}


/*
 * Main functions for receiving and parsing
 */

int tcp_init(void)
{
	int fd, rv;
	struct sockaddr_in srvsa;
	struct in_addr ia;

	rv = inet_pton(AF_INET, settings.tcp_addr, &ia);
	if (rv <= 0)
		return -1;

	srvsa.sin_family = AF_INET;
	srvsa.sin_addr.s_addr = ia.s_addr;
	srvsa.sin_port = htons(settings.tcp_port);


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
	int newfd;
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

	tcpsock->fd = newfd;
	tcpsock->evt = new_event;
	tcpsock->buf = NULL;
	tcpsock->pktsize = 0;
	tcpsock->len = 0;
	tcpsock->req = NULL;
	tcpsock->excess = 0;

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
		 * which is 64k; it will be freed by process_buf(). */
		bsize = 68 * 1024;
		buf = malloc(bsize);
		if (buf == NULL) {
			goto error_exit;
		}

		rv = recv(fd, buf, bsize, 0);
		if (rv < 0 && errno == EAGAIN) {
			/* We were awoken but have no data to read, so we do
			 * nothing */
			free(buf);
			return;
		} else if (rv <= 0) {
			/* Orderly shutdown or error; close the file
			 * descriptor in either case. */
			free(buf);
			goto error_exit;
		}

		req = build_req(tcpsock);
		process_buf(tcpsock, req, buf, rv);

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

		process_buf(tcpsock, tcpsock->req, tcpsock->buf, tcpsock->len);
	}

	return;

error_exit:
	close(fd);
	event_del(tcpsock->evt);
	tcp_socket_free(tcpsock);
	return;
}


/* Main message unwrapping */
static void process_buf(struct tcp_socket *tcpsock, struct req_info *req,
		unsigned char *buf, size_t len)
{
	uint32_t totaltoget = 0;

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

	if (totaltoget > len) {
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

	} else if (totaltoget < len) {
		/* Got more than one message in the same recv(); save the
		 * amount of bytes exceeding so we can process it later. */
		tcpsock->excess = len - totaltoget;
		len = totaltoget;
	}

	printf("parsing\n");

	/* The buffer is complete, parse it as usual. */

	if (parse_message(req, buf + 4, len - 4)) {
		goto exit;
	} else {
		goto error_exit;
	}


exit:
	printf("pm exit\n");
	/* We completed the read successfuly. buf and req were allocated by
	 * tcp_recv(), but they are freed here only after we have fully parsed
	 * the message. */

	if (tcpsock->excess) {
		/* If there are buffer leftovers (because there was more than
		 * one message on a recv()), leave the buffer, move the
		 * leftovers to the beginning, adjust the numbers and parse
		 * recursively. */
		memmove(buf, buf + len, tcpsock->excess);
		tcpsock->len = tcpsock->excess;
		tcpsock->excess = 0;

		/* The req is no longer needed here, we create a new one. */
		free(req);

		/* Build a new req just like when we first recv(). */
		req = build_req(tcpsock);
		process_buf(tcpsock, req, buf, len);
		return;

	} else {
		if (tcpsock->buf) {
			tcpsock->buf = NULL;
			tcpsock->len = 0;
			tcpsock->pktsize = 0;
			tcpsock->req = NULL;
		}

		free(buf);
		free(req);
	}
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


