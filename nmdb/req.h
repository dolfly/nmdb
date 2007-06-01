
#ifndef _REQ_H
#define _REQ_H

#include <stdint.h>		/* uint32_t */
#include <sys/types.h>		/* size_t */
#include <sys/socket.h>		/* socklen_t */


/* req_info types, according to the protocol */
#define REQTYPE_TIPC 1
#define REQTYPE_TCP 2


struct req_info {
	/* network information */
	int fd;
	int type;

	struct sockaddr *clisa;
	socklen_t clilen;

	/* operation information */
	uint32_t id;
	uint32_t cmd;
	unsigned char *payload;
	size_t psize;

	/* operations */
	void (*reply_err)(struct req_info *req, uint32_t reply);
	void (*reply_get)(struct req_info *req, uint32_t reply,
			unsigned char *val, size_t vsize);
	void (*reply_set)(struct req_info *req, uint32_t reply);
	void (*reply_del)(struct req_info *req, uint32_t reply);
	void (*reply_cas)(struct req_info *req, uint32_t reply);
};

#endif

