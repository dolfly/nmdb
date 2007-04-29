
#ifndef _MYTIPC_H
#define _MYTIPC_H

#include <stdint.h>		/* uint32_t */
#include <sys/types.h>		/* size_t */
#include <sys/socket.h>		/* socklen_t */
#include <linux/tipc.h>		/* sockaddr_tipc */

struct req_info {
	/* network information */
	int fd;
	struct sockaddr_tipc *clisa;
	socklen_t clilen;

	/* operation information */
	uint32_t id;
	uint32_t cmd;
	unsigned char *payload;
	size_t psize;
};


int tipc_init(void);
void tipc_recv(int fd, short event, void *arg);

void tipc_reply_err(struct req_info *req, uint32_t reply);
void tipc_reply_get(struct req_info *req, uint32_t reply,
		unsigned char *val, size_t vsize);
void tipc_reply_set(struct req_info *req, uint32_t reply);
void tipc_reply_del(struct req_info *req, uint32_t reply);
void tipc_reply_cas(struct req_info *req, uint32_t reply);

#endif

