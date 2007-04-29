
#ifndef _NET_CONST_H
#define _NET_CONST_H

/*
 * Local network constants.
 * Isolated so it's shared between the server and the library code.
 */

/* TIPC server type and instance -- Hardcoded for now. */
#define SERVER_TYPE 26001
#define SERVER_INST 10

/* Protocol version, for checking in the network header. */
#define PROTO_VER 1

/* Network requests */
#define REQ_CACHE_GET		0x101
#define REQ_CACHE_SET		0x102
#define REQ_CACHE_DEL		0x103
#define REQ_GET			0x104
#define REQ_SET_SYNC		0x105
#define REQ_DEL_SYNC		0x106
#define REQ_SET_ASYNC		0x107
#define REQ_DEL_ASYNC		0x108
#define REQ_CACHE_CAS		0x109
#define REQ_CAS			0x110

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


#endif

