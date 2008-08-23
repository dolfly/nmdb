
#include <stdlib.h>		/* malloc() */
#include <stdint.h>		/* uint32_t and friends */
#include <string.h>		/* memcpy() */
#include <arpa/inet.h>		/* htonl() and friends */

#include "parse.h"
#include "req.h"
#include "queue.h"
#include "net-const.h"
#include "common.h"
#include "netutils.h"


static void parse_get(struct req_info *req);
static void parse_set(struct req_info *req);
static void parse_del(struct req_info *req);
static void parse_cas(struct req_info *req);
static void parse_incr(struct req_info *req);
static void parse_stats(struct req_info *req);


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

	e->req->clisa = malloc(req->clilen);
	if (e->req->clisa == NULL) {
		queue_entry_free(e);
		return NULL;
	}
	memcpy(e->req->clisa, req->clisa, req->clilen);

	/* clear out unused fields */
	e->req->payload = NULL;
	e->req->psize = 0;

	return e;
}


/* Creates a new queue entry and puts it into the queue. Returns 1 if success,
 * 0 if memory error. */
static int put_in_queue_long(struct req_info *req,
		uint32_t operation, int sync,
		const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize,
		const unsigned char *newval, size_t nvsize)
{
	struct queue_entry *e;

	e = make_queue_long_entry(req, operation, key, ksize, val, vsize,
			newval, nvsize);
	if (e == NULL) {
		return 0;
	}
	queue_lock(op_queue);
	queue_put(op_queue, e);
	queue_unlock(op_queue);

	if (sync) {
		/* Signal the DB thread it has work only if it's a
		 * synchronous operation, asynchronous don't mind
		 * waiting. It does have a measurable impact on
		 * performance (2083847usec vs 2804973usec for sets on
		 * "test2d 100000 10 10"). */
		queue_signal(op_queue);
	}

	return 1;
}

/* Like put_in_queue_long() but with few parameters because most actions do
 * not need newval. */
static int put_in_queue(struct req_info *req,
		uint32_t operation, int sync,
		const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return put_in_queue_long(req, operation, sync, key, ksize, val, vsize,
			NULL, 0);
}


/* Parse an incoming message. Note that the protocol might have sent this
 * directly over the network (ie. TIPC) or might have wrapped it around (ie.
 * TCP). Here we only deal with the clean, stripped, non protocol-specific
 * message. */
int parse_message(struct req_info *req,
		const unsigned char *buf, size_t len)
{
	uint32_t hdr, ver, id;
	uint16_t cmd, flags;
	const unsigned char *payload;
	size_t psize;

	/* The header is:
	 * 4 bytes	Version + ID
	 * 2 bytes	Command
	 * 2 bytes	Flags
	 * Variable 	Payload
	 */

	hdr = * ((uint32_t *) buf);
	hdr = htonl(hdr);

	/* FIXME: little endian-only */
	ver = (hdr & 0xF0000000) >> 28;
	id = hdr & 0x0FFFFFFF;
	req->id = id;

	cmd = ntohs(* ((uint16_t *) buf + 2));
	flags = ntohs(* ((uint16_t *) buf + 3));

	if (ver != PROTO_VER) {
		stats.net_version_mismatch++;
		req->reply_err(req, ERR_VER);
		return 0;
	}

	/* We define payload as the stuff after buf. But be careful because
	 * there might be none (if len == 1). Doing the pointer arithmetic
	 * isn't problematic, but accessing the payload should be done only if
	 * we know we have enough data. */
	payload = buf + 8;
	psize = len - 8;

	/* Store the id encoded in network byte order, so that we don't have
	 * to calculate it at send time. */
	req->id = htonl(id);
	req->cmd = cmd;
	req->flags = flags;
	req->payload = payload;
	req->psize = psize;

	if (cmd == REQ_GET) {
		parse_get(req);
	} else if (cmd == REQ_SET) {
		parse_set(req);
	} else if (cmd == REQ_DEL) {
		parse_del(req);
	} else if (cmd == REQ_CAS) {
		parse_cas(req);
	} else if (cmd == REQ_INCR) {
		parse_incr(req);
	} else if (cmd == REQ_STATS) {
		parse_stats(req);
	} else {
		stats.net_unk_req++;
		req->reply_err(req, ERR_UNKREQ);
	}

	return 1;
}


/* Small macros used to handle flags in the parse_*() functions */
#define FILL_CACHE_FLAG(OP) \
	do { \
		cache_only = req->flags & FLAGS_CACHE_ONLY; \
		if (cache_only) stats.cache_##OP++; \
		else stats.db_##OP++; \
	} while (0)

#define FILL_SYNC_FLAG() \
	do { \
		sync = req->flags & FLAGS_SYNC; \
	} while(0)


static void parse_get(struct req_info *req)
{
	int hit, cache_only, rv;
	const unsigned char *key;
	uint32_t ksize;
	unsigned char *val = NULL;
	size_t vsize = 0;

	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);
	if (req->psize < ksize) {
		stats.net_broken_req++;
		req->reply_err(req, ERR_BROKEN);
		return;
	}

	FILL_CACHE_FLAG(get);

	key = req->payload + sizeof(uint32_t);

	hit = cache_get(cache_table, key, ksize, &val, &vsize);

	if (cache_only && !hit) {
		stats.cache_misses++;
		req->reply_mini(req, REP_CACHE_MISS);
		return;
	} else if (!cache_only && !hit) {
		rv = put_in_queue(req, REQ_GET, 1, key, ksize, NULL, 0);
		if (!rv) {
			req->reply_err(req, ERR_MEM);
			return;
		}
		return;
	} else {
		stats.cache_hits++;
		req->reply_long(req, REP_CACHE_HIT, val, vsize);
		return;
	}
}

static void parse_set(struct req_info *req)
{
	int rv, cache_only, sync;
	const unsigned char *key, *val;
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
		req->reply_err(req, ERR_BROKEN);
		return;
	}

	FILL_CACHE_FLAG(set);
	FILL_SYNC_FLAG();

	key = req->payload + sizeof(uint32_t) * 2;
	val = key + ksize;

	rv = cache_set(cache_table, key, ksize, val, vsize);
	if (!rv) {
		req->reply_err(req, ERR_MEM);
		return;
	}

	if (!cache_only) {
		rv = put_in_queue(req, REQ_SET, sync, key, ksize, val, vsize);
		if (!rv) {
			req->reply_err(req, ERR_MEM);
			return;
		}

		if (!sync) {
			req->reply_mini(req, REP_OK);
		}

		return;
	} else {
		req->reply_mini(req, REP_OK);
	}

	return;
}


static void parse_del(struct req_info *req)
{
	int hit, cache_only, sync, rv;
	const unsigned char *key;
	uint32_t ksize;

	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);
	if (req->psize < ksize) {
		stats.net_broken_req++;
		req->reply_err(req, ERR_BROKEN);
		return;
	}

	FILL_CACHE_FLAG(del);
	FILL_SYNC_FLAG();

	key = req->payload + sizeof(uint32_t);

	hit = cache_del(cache_table, key, ksize);

	if (cache_only && hit) {
		req->reply_mini(req, REP_OK);
	} else if (cache_only && !hit) {
		req->reply_mini(req, REP_NOTIN);
	} else if (!cache_only) {
		rv = put_in_queue(req, REQ_DEL, sync, key, ksize, NULL, 0);
		if (!rv) {
			req->reply_err(req, ERR_MEM);
			return;
		}

		if (!sync) {
			req->reply_mini(req, REP_OK);
		}

		return;
	}

	return;
}

static void parse_cas(struct req_info *req)
{
	int rv, cache_only;
	const unsigned char *key, *oldval, *newval;
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
		req->reply_err(req, ERR_BROKEN);
		return;
	}

	FILL_CACHE_FLAG(cas);

	key = req->payload + sizeof(uint32_t) * 3;
	oldval = key + ksize;
	newval = oldval + ovsize;

	rv = cache_cas(cache_table, key, ksize, oldval, ovsize,
			newval, nvsize);
	if (rv == 0) {
		/* If the cache doesn't match, there is no need to bother the
		 * DB even if we were asked to impact. */
		req->reply_mini(req, REP_NOMATCH);
		return;
	}

	if (cache_only) {
		if (rv == -1) {
			req->reply_mini(req, REP_NOTIN);
			return;
		} else {
			req->reply_mini(req, REP_OK);
			return;
		}
	} else {
		/* !cache_only and the key is either not in the cache, or
		 * cache_cas() was successful. We now need to queue the CAS in
		 * the database. */
		rv = put_in_queue_long(req, REQ_CAS, 1, key, ksize,
				oldval, ovsize, newval, nvsize);
		if (!rv) {
			req->reply_err(req, ERR_MEM);
			return;
		}
	}
	return;
}

static void parse_incr(struct req_info *req)
{
	int cres, cache_only, rv;
	const unsigned char *key;
	uint32_t ksize;
	int64_t increment, newval;
	const int max = 65536;

	/* Request format:
	 * 4		ksize
	 * ksize	key
	 * 8		increment (big endian int64_t)
	 */
	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);

	/* Sanity check on sizes:
	 * - ksize + 8 must be < req->psize
	 * - ksize + 8 must be < 2^16 = 64k
	 */
	if ( (req->psize < ksize + 8) || ((ksize + 8) > max)) {
		stats.net_broken_req++;
		req->reply_err(req, ERR_BROKEN);
		return;
	}

	FILL_CACHE_FLAG(incr);

	key = req->payload + sizeof(uint32_t);
	increment = ntohll( * (int64_t *) (key + ksize) );

	cres = cache_incr(cache_table, key, ksize, increment, &newval);
	if (cres == -3) {
		req->reply_err(req, ERR_MEM);
		return;
	} else if (cres == -2) {
		/* the value was not NULL terminated */
		req->reply_mini(req, REP_NOMATCH);
		return;
	}

	if (!cache_only) {
		/* at this point, the cache_incr() was either successful or a
		 * miss, but we don't really care */
		rv = put_in_queue(req, REQ_INCR, 1, key, ksize,
				(unsigned char *) &increment,
				sizeof(increment));
		if (!rv) {
			req->reply_err(req, ERR_MEM);
			return;
		}
	} else {
		if (cres == -1) {
			req->reply_mini(req, REP_NOTIN);
		} else {
			newval = htonll(newval);
			req->reply_long(req, REP_OK,
					(unsigned char *) &newval,
					sizeof(newval));
		}
	}

	return;
}


static void parse_stats(struct req_info *req)
{
	int i;
	uint64_t response[STATS_REPLY_SIZE];

	/* The packet is just the request, there's no payload. We need to
	 * reply with the stats structure.
	 * The response structure is just several uint64_t packed together,
	 * each one corresponds to a single value of the stats structure. */

	/* We define a macro to do the assignment easily; it's not nice, but
	 * it's more portable than using a packed struct */
	i = 0;
	#define fcpy(field) \
		do { response[i] = htonll(stats.field); i++; } while(0)


	fcpy(cache_get);
	fcpy(cache_set);
	fcpy(cache_del);
	fcpy(cache_cas);
	fcpy(cache_incr);

	fcpy(db_get);
	fcpy(db_set);
	fcpy(db_del);
	fcpy(db_cas);
	fcpy(db_incr);

	fcpy(cache_hits);
	fcpy(cache_misses);

	fcpy(db_hits);
	fcpy(db_misses);

	fcpy(msg_tipc);
	fcpy(msg_tcp);
	fcpy(msg_udp);
	fcpy(msg_sctp);

	fcpy(net_version_mismatch);
	fcpy(net_broken_req);
	fcpy(net_unk_req);

	req->reply_long(req, REP_OK, (unsigned char *) response,
			sizeof(response));

	return;
}


