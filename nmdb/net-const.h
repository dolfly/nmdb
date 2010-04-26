
#ifndef _NET_CONST_H
#define _NET_CONST_H

/*
 * Local network constants.
 * Isolated so it's shared between the server and the library code.
 */

/* TIPC server type (hardcoded) and default instance. */
#define TIPC_SERVER_TYPE 26001
#define TIPC_SERVER_INST 10

/* TCP default listen address and port. */
#define TCP_SERVER_ADDR "0.0.0.0"
#define TCP_SERVER_PORT 26010

/* UDP default listen address and port. */
#define UDP_SERVER_ADDR "0.0.0.0"
#define UDP_SERVER_PORT 26010

/* SCTP default listen address and port. */
#define SCTP_SERVER_ADDR "0.0.0.0"
#define SCTP_SERVER_PORT 26010

/* Protocol version, for checking in the network header. */
#define PROTO_VER 1

/* Network requests */
#define REQ_GET			0x101
#define REQ_SET			0x102
#define REQ_DEL			0x103
#define REQ_CAS			0x104
#define REQ_INCR		0x105
#define REQ_STATS		0x106
#define REQ_FIRSTKEY		0x107
#define REQ_NEXTKEY		0x108

/* Possible request flags (which can be applied to the documented requests) */
#define FLAGS_CACHE_ONLY	1	/* get, set, del, cas, incr */
#define FLAGS_SYNC		2	/* set, del */

/* Network replies (different namespace from requests) */
#define REP_ERR			0x800
#define REP_CACHE_HIT		0x801
#define REP_CACHE_MISS		0x802
#define REP_OK			0x803
#define REP_NOTIN		0x804
#define REP_NOMATCH		0x805

/* Network error replies */
#define ERR_VER			0x101	/* Version mismatch */
#define ERR_SEND		0x102	/* Error sending data */
#define ERR_BROKEN		0x103	/* Broken request */
#define ERR_UNKREQ		0x104	/* Unknown request */
#define ERR_MEM			0x105	/* Memory allocation error */
#define ERR_DB			0x106	/* Database error */
#define ERR_RO			0x107	/* Server in read-only mode */


#endif

