
#if ENABLE_SCTP

#include <sys/types.h>		/* socket defines */
#include <sys/socket.h>		/* socket functions */
#include <stdlib.h>		/* malloc() */
#include <stdint.h>		/* uint32_t and friends */
#include <arpa/inet.h>		/* htonls() and friends */
#include <string.h>		/* memcpy() */
#include <unistd.h>		/* close() */

#include <netinet/sctp.h>	/* SCTP stuff */
#include <netdb.h>		/* gethostbyname() */

#include "nmdb.h"
#include "net-const.h"
#include "internal.h"


/* Used internally to really add the server once we have an IP address. */
static int add_sctp_server_addr(nmdb_t *db, in_addr_t *inetaddr, int port)
{
	int fd, rv;
	struct nmdb_srv *newsrv, *newarray;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0)
		return 0;

	/* Disable Nagle algorithm because we often send small
	 * packets. Huge gain in performance. */
	rv = 1;
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, &rv, sizeof(rv)) < 0 ) {
		close(fd);
		return 0;
	}

	newarray = realloc(db->servers,
			sizeof(struct nmdb_srv) * (db->nservers + 1));
	if (newarray == NULL) {
		close(fd);
		return 0;
	}

	db->servers = newarray;
	db->nservers++;

	if (port < 0)
		port = SCTP_SERVER_PORT;

	newsrv = &(db->servers[db->nservers - 1]);

	newsrv->fd = fd;
	newsrv->info.in.srvsa.sin_family = AF_INET;
	newsrv->info.in.srvsa.sin_port = htons(port);
	newsrv->info.in.srvsa.sin_addr.s_addr = *inetaddr;
	newsrv->info.in.srvlen = sizeof(struct sockaddr_in);

	newsrv->type = SCTP_CONN;

	/* keep the list sorted by port, so we can do a reliable selection */
	qsort(db->servers, db->nservers, sizeof(struct nmdb_srv),
			compare_servers);

	return 1;
}

/* Same as nmdb_add_tcp_server() but for SCTP. */
int nmdb_add_sctp_server(nmdb_t *db, const char *addr, int port)
{
	int rv;
	struct hostent *he;
	struct in_addr ia;

	/* We try to resolve and then pass it to add_sctp_server_addr(). */
	rv = inet_pton(AF_INET, addr, &ia);
	if (rv <= 0) {
		he = gethostbyname(addr);
		if (he == NULL)
			return 0;

		ia.s_addr = *( (in_addr_t *) (he->h_addr_list[0]) );
	}

	return add_sctp_server_addr(db, &(ia.s_addr), port);
}

int sctp_srv_send(struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize)
{
	ssize_t rv;
	rv = sendto(srv->fd, buf, bsize, 0,
			(struct sockaddr *) &(srv->info.in.srvsa),
			srv->info.in.srvlen);
	if (rv <= 0)
		return 0;
	return 1;
}

/* Used internally to get and parse replies from the server. */
uint32_t sctp_get_rep(struct nmdb_srv *srv,
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
/* Stubs to use when SCTP is not enabled. */

#include <stdint.h>
#include "nmdb.h"

int nmdb_add_sctp_server(nmdb_t *db, const char *addr, int port)
{
	return 0;
}

int sctp_srv_send(struct nmdb_srv *srv,
		const unsigned char *buf, size_t bsize)
{
	return 0;
}

uint32_t sctp_get_rep(struct nmdb_srv *srv,
		unsigned char *buf, size_t bsize,
		unsigned char **payload, size_t *psize)
{
	return -1;
}

#endif /* ENABLE_SCTP */

