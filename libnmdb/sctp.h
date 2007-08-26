
#ifndef _SCTP_H
#define _SCTP_H

int sctp_srv_send(struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize);
uint32_t sctp_get_rep(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize);

#endif

