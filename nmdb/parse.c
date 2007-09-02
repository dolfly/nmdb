
#include <stdlib.h>		/* malloc() */
#include <stdint.h>		/* uint32_t and friends */
#include <string.h>		/* memcpy() */
#include <arpa/inet.h>		/* htonl() and friends */


#include "parse.h"
#include "req.h"
#include "queue.h"
#include "net-const.h"
#include "common.h"


static void parse_get(struct req_info *req, int impact_db);
static void parse_set(struct req_info *req, int impact_db, int async);
static void parse_del(struct req_info *req, int impact_db, int async);
static void parse_cas(struct req_info *req, int impact_db);
static void parse_incr(struct req_info *req, int impact_db);


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

/* Like make_queue_long_entry() but with few parameters because most actions
 * do not need newval. */
static struct queue_entry *make_queue_entry(struct req_info *req,
		uint32_t operation, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	return make_queue_long_entry(req, operation, key, ksize, val, vsize,
			NULL, 0);
}


/* Parse an incoming message. Note that the protocol might have sent this
 * directly over the network (ie. TIPC) or might have wrapped it around (ie.
 * TCP). Here we only deal with the clean, stripped, non protocol-specific
 * message. */
int parse_message(struct req_info *req,
		const unsigned char *buf, size_t len)
{
	uint32_t hdr, ver, id, cmd;
	const unsigned char *payload;
	size_t psize;

	/* The header is:
	 * 4 bytes	Version + ID
	 * 4 bytes	Command
	 * Variable 	Payload
	 */

	hdr = * ((uint32_t *) buf);
	hdr = htonl(hdr);

	/* FIXME: little endian-only */
	ver = (hdr & 0xF0000000) >> 28;
	id = hdr & 0x0FFFFFFF;
	req->id = id;

	cmd = ntohl(* ((uint32_t *) buf + 1));

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
	else if (cmd == REQ_CACHE_INCR)
		parse_incr(req, 0);
	else if (cmd == REQ_INCR)
		parse_incr(req, 1);
	else {
		stats.net_unk_req++;
		req->reply_err(req, ERR_UNKREQ);
	}

	return 1;
}


static void parse_get(struct req_info *req, int impact_db)
{
	int hit;
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

	key = req->payload + sizeof(uint32_t);

	hit = cache_get(cache_table, key, ksize, &val, &vsize);

	if (!hit && !impact_db) {
		req->reply_mini(req, REP_CACHE_MISS);
		return;
	} else if (!hit && impact_db) {
		struct queue_entry *e;
		e = make_queue_entry(req, REQ_GET, key, ksize, NULL, 0);
		if (e == NULL) {
			req->reply_err(req, ERR_MEM);
			return;
		}
		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);
		queue_signal(op_queue);
		return;
	} else {
		req->reply_long(req, REP_CACHE_HIT, val, vsize);
		return;
	}
}


static void parse_set(struct req_info *req, int impact_db, int async)
{
	int rv;
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

	key = req->payload + sizeof(uint32_t) * 2;
	val = key + ksize;

	rv = cache_set(cache_table, key, ksize, val, vsize);
	if (!rv) {
		req->reply_err(req, ERR_MEM);
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
			req->reply_err(req, ERR_MEM);
			return;
		}
		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);

		if (async) {
			req->reply_mini(req, REP_OK);
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
		req->reply_mini(req, REP_OK);
	}

	return;
}


static void parse_del(struct req_info *req, int impact_db, int async)
{
	int hit;
	const unsigned char *key;
	uint32_t ksize;

	ksize = * (uint32_t *) req->payload;
	ksize = ntohl(ksize);
	if (req->psize < ksize) {
		stats.net_broken_req++;
		req->reply_err(req, ERR_BROKEN);
		return;
	}

	key = req->payload + sizeof(uint32_t);

	hit = cache_del(cache_table, key, ksize);

	if (!impact_db && hit) {
		req->reply_mini(req, REP_OK);
	} else if (!impact_db && !hit) {
		req->reply_mini(req, REP_NOTIN);
	} else if (impact_db) {
		struct queue_entry *e;
		uint32_t request;

		request = REQ_DEL_SYNC;
		if (async)
			request = REQ_DEL_ASYNC;

		e = make_queue_entry(req, request, key, ksize, NULL, 0);
		if (e == NULL) {
			req->reply_err(req, ERR_MEM);
			return;
		}
		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);

		if (async) {
			req->reply_mini(req, REP_OK);
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

	if (!impact_db) {
		if (rv == -1) {
			req->reply_mini(req, REP_NOTIN);
			return;
		} else {
			req->reply_mini(req, REP_OK);
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
			req->reply_err(req, ERR_MEM);
			return;
		}

		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);
		queue_signal(op_queue);
	}
	return;
}


/* ntohll() is not standard, so we define it using an UGLY trick because there
 * is no standard way to check for endianness at runtime! */
static uint64_t ntohll(uint64_t x)
{
	static int endianness = 0;

	/* determine the endianness by checking how htonl() behaves; use -1
	 * for little endian and 1 for big endian */
	if (endianness == 0) {
		if (htonl(1) == 1)
			endianness = 1;
		else
			endianness = -1;
	}

	if (endianness == 1) {
		/* big endian */
		return x;
	}

	/* little endian */
	return ( ntohl( (x >> 32) & 0xFFFFFFFF ) | \
			( (uint64_t) ntohl(x & 0xFFFFFFFF) ) << 32 );
}


static void parse_incr(struct req_info *req, int impact_db)
{
	int cres;
	const unsigned char *key;
	uint32_t ksize;
	int64_t increment;
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

	key = req->payload + sizeof(uint32_t);
	increment = ntohll( * (int64_t *) (key + ksize) );

	cres = cache_incr(cache_table, key, ksize, increment);
	if (cres == -3) {
		req->reply_err(req, ERR_MEM);
		return;
	} else if (cres == -2) {
		/* the value was not NULL terminated */
		req->reply_mini(req, REP_NOMATCH);
		return;
	}

	if (impact_db) {
		struct queue_entry *e;

		/* at this point, the cache_incr() was either successful or a
		 * miss, but we don't really care */

		e = make_queue_entry(req, REQ_INCR, key, ksize,
				(unsigned char *) &increment,
				sizeof(increment));
		if (e == NULL) {
			req->reply_err(req, ERR_MEM);
			return;
		}
		queue_lock(op_queue);
		queue_put(op_queue, e);
		queue_unlock(op_queue);

		queue_signal(op_queue);
	} else {
		if (cres == -1)
			req->reply_mini(req, REP_NOTIN);
		else
			req->reply_mini(req, REP_OK);
	}

	return;
}


