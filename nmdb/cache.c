
/* Generic cache layer.
 * It's a hash table with cache-style properties, keeping a (non-precise) size
 * and using a natural, per-chain LRU to do cleanups.
 * Cleanups are performed in place, when cache_set() gets called.
 */

#include <sys/types.h>		/* for size_t */
#include <stdint.h>		/* for [u]int*_t */
#include <stdlib.h>		/* for malloc() */
#include <string.h>		/* for memcpy()/memcmp() */
#include <stdio.h>		/* snprintf() */
#include "hash.h"		/* hash() */
#include "cache.h"


struct cache *cache_create(size_t numobjs, unsigned int flags)
{
	size_t i, j;
	struct cache *cd;
	struct cache_chain *c;

	cd = (struct cache *) malloc(sizeof(struct cache));
	if (cd == NULL)
		return NULL;

	cd->flags = flags;

	/* We calculate the hash size so we have 4 objects per bucket; 4 being
	 * an arbitrary number. It's long enough to make LRU useful, and small
	 * enough to make lookups fast. */
	cd->numobjs = numobjs;
	cd->hashlen = numobjs / CHAINLEN;

	cd->table = (struct cache_chain *)
			malloc(sizeof(struct cache_chain) * cd->hashlen);
	if (cd->table == NULL) {
		free(cd);
		return NULL;
	}

	for (i = 0; i < cd->hashlen; i++) {
		c = cd->table + i;
		c->len = 0;
		c->first = NULL;
		c->last = NULL;
		for (j = 0; j < CHAINLEN; j++) {
			memset(&(c->entries[j]), 0,
				sizeof(struct cache_entry));

			/* mark them as unused */
			c->entries[j].ksize = -1;
			c->entries[j].vsize = -1;
		}
	}

	return cd;
}

int cache_free(struct cache *cd)
{
	size_t i;
	struct cache_chain *c;
	struct cache_entry *e, *n;

	for (i = 0; i < cd->hashlen; i++) {
		c = cd->table + i;
		if (c->first == NULL)
			continue;

		e = c->first;
		while (e != NULL) {
			n = e->next;
			free(e->key);
			free(e->val);
			e = n;
		}
	}

	free(cd->table);
	free(cd);
	return 1;
}

static struct cache_entry *alloc_entry(struct cache_chain *c)
{
	int i;

	for (i = 0; i < CHAINLEN; i++) {
		if (c->entries[i].ksize == -1)
			return &(c->entries[i]);
	}

	return NULL;
}

static void free_entry(struct cache_entry *e)
{
	e->key = NULL;
	e->ksize = -1;
	e->val = NULL;
	e->vsize = -1;
}


/* Looks up the given key in the chain. Returns NULL if not found, or a
 * pointer to the cache entry if it is. The chain can be empty. */
static struct cache_entry *find_in_chain(struct cache_chain *c,
		const unsigned char *key, size_t ksize)
{
	struct cache_entry *e;

	for (e = c->first; e != NULL; e = e->next) {
		if (ksize != e->ksize) {
			continue;
		}
		if (memcmp(key, e->key, ksize) == 0) {
			break;
		}
	}

	/* e will be either the found chain or NULL */
	return e;
}


/* Looks up the given key in the cache. Returns NULL if not found, or a
 * pointer to the cache entry if it is. Useful to avoid doing the calculation
 * in the open when the cache chain will not be needed. */
static struct cache_entry *find_in_cache(struct cache *cd,
		const unsigned char *key, size_t ksize)
{
	uint32_t h;
	struct cache_chain *c;

	h = hash(key, ksize) % cd->hashlen;
	c = cd->table + h;

	return find_in_chain(c, key, ksize);
}


/* Gets the matching value for the given key.  Returns 0 if no match was
 * found, or 1 otherwise. */
int cache_get(struct cache *cd, const unsigned char *key, size_t ksize,
		unsigned char **val, size_t *vsize)
{
	struct cache_entry *e;

	e = find_in_cache(cd, key, ksize);

	if (e == NULL) {
		*val = NULL;
		*vsize = 0;
		return 0;
	}

	*val = e->val;
	*vsize = e->vsize;

	return 1;
}

/* Creates a new cache entry, with the given key and value */
static struct cache_entry *new_entry(struct cache_chain *c,
		const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	struct cache_entry *new;

	new = alloc_entry(c);
	if (new == NULL) {
		return NULL;
	}

	new->ksize = ksize;
	new->vsize = vsize;

	new->key = malloc(ksize);
	if (new->key == NULL) {
		free(new);
		return NULL;
	}
	memcpy(new->key, key, ksize);

	new->val = malloc(vsize);
	if (new->val == NULL) {
		free(new->key);
		free_entry(new);
		return NULL;
	}
	memcpy(new->val, val, vsize);
	new->prev = NULL;
	new->next = NULL;

	return new;
}

static int insert_in_full_chain(struct cache_chain *c,
		const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	/* To insert in a full chain, we evict the last entry and place the
	 * new one at the beginning.
	 *
	 * As an optimization, we avoid re-allocating the entry by reusing the
	 * last one, and when possible we reuse its key and value as well. */
	struct cache_entry *e = c->last;

	if (ksize == e->ksize) {
		memcpy(e->key, key, ksize);
	} else {
		free(e->key);
		e->key = malloc(ksize);
		if (e->key == NULL) {
			goto error;
		}
		e->ksize = ksize;
		memcpy(e->key, key, ksize);
	}

	if (vsize == e->vsize) {
		memcpy(e->val, val, vsize);
	} else {
		free(e->val);
		e->val = malloc(vsize);
		if (e->val == NULL) {
			goto error;
		}
		e->vsize = vsize;
		memcpy(e->val, val, vsize);
	}

	/* move the entry from the last to the first position */
	c->last = e->prev;
	c->last->next = NULL;

	e->prev = NULL;
	e->next = c->first;

	c->first->prev = e;
	c->first = e;

	return 0;

error:
	/* on errors, remove the entry just in case */
	c->last = e->prev;
	c->last->next = NULL;
	free(e->key);
	free(e->val);
	free_entry(e);

	return -1;
}


int cache_set(struct cache *cd, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	uint32_t h = 0;
	struct cache_chain *c;
	struct cache_entry *e, *new;
	unsigned char *v;

	h = hash(key, ksize) % cd->hashlen;
	c = cd->table + h;

	e = find_in_chain(c, key, ksize);

	if (e == NULL) {
		if (c->len == CHAINLEN) {
			/* chain is full */
			if (insert_in_full_chain(c, key, ksize,
					val, vsize) != 0)
				return -1;
		} else {
			new = new_entry(c, key, ksize, val, vsize);
			if (new == NULL)
				return -1;

			if (c->len == 0) {
				/* line is empty, just put it there */
				c->first = new;
				c->last = new;
				c->len = 1;
			} else if (c->len < CHAINLEN) {
				/* slots are still available, put the entry
				 * first */
				new->next = c->first;
				c->first->prev = new;
				c->first = new;
				c->len += 1;
			}
		}
	} else {
		/* we've got a match, just replace the value in place */
		if (vsize == e->vsize) {
			memcpy(e->val, val, vsize);
		} else {
			v = malloc(vsize);
			if (v == NULL)
				return -1;

			free(e->val);
			e->val = v;
			e->vsize = vsize;
			memcpy(e->val, val, vsize);
		}

		/* promote the entry to the top of the list if necessary */
		if (c->first != e) {
			if (c->last == e)
				c->last = e->prev;

			e->prev->next = e->next;
			if (e->next != NULL)
				e->next->prev = e->prev;
			e->prev = NULL;
			e->next = c->first;
			c->first->prev = e;
			c->first = e;
		}
	}

	return 0;
}


int cache_del(struct cache *cd, const unsigned char *key, size_t ksize)
{

	int rv = 1;
	uint32_t h = 0;
	struct cache_chain *c;
	struct cache_entry *e;

	h = hash(key, ksize) % cd->hashlen;
	c = cd->table + h;

	e = find_in_chain(c, key, ksize);

	if (e == NULL) {
		rv = 0;
		goto exit;
	}

	if (c->first == e) {
		c->first = e->next;
		if (e->next != NULL)
			e->next->prev = NULL;
	} else {
		e->prev->next = e->next;
		if (e->next != NULL)
			e->next->prev = e->prev;
	}

	if (c->last == e) {
		c->last = e->prev;
	}

	free(e->key);
	free(e->val);
	free_entry(e);

	c->len -= 1;

exit:
	return rv;
}


/* Performs a cache compare-and-swap.
 * Returns -3 if there was an error, -2 if the key is not in the cache, -1 if
 * the old value does not match, and 0 if the CAS was successful. */
int cache_cas(struct cache *cd, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize)
{
	struct cache_entry *e;
	unsigned char *buf;

	e = find_in_cache(cd, key, ksize);

	if (e == NULL)
		return -2;

	if (e->vsize != ovsize)
		return -1;

	if (memcmp(e->val, oldval, ovsize) != 0)
		return -1;

	if (ovsize == nvsize) {
		/* since they have the same size, avoid the malloc() and just
		 * copy the new value */
		memcpy(e->val, newval, nvsize);
	} else {
		buf = malloc(nvsize);
		if (buf == NULL)
			return -3;

		memcpy(buf, newval, nvsize);
		free(e->val);
		e->val = buf;
		e->vsize = nvsize;
	}

	return 0;
}


/* Increment the value associated with the given key by the given increment.
 * The increment is a signed 64 bit value, and the value size must be >= 8
 * bytes.
 * Returns:
 *    0 if the increment succeeded.
 *   -1 if the value was not in the cache.
 *   -2 if the value was not null terminated.
 *   -3 if there was a memory error.
 *
 * The new value will be set in the newval parameter if the increment was
 * successful.
 */
int cache_incr(struct cache *cd, const unsigned char *key, size_t ksize,
		int64_t increment, int64_t *newval)
{
	unsigned char *val;
	int64_t intval;
	size_t vsize;
	struct cache_entry *e;

	e = find_in_cache(cd, key, ksize);

	if (e == NULL)
		return -1;

	val = e->val;
	vsize = e->vsize;

	/* The value must be a 0-terminated string, otherwise strtoll might
	 * cause a segmentation fault. Note that val should never be NULL, but
	 * it doesn't hurt to check just in case */
	if (val == NULL || val[vsize - 1] != '\0')
		return -2;

	intval = strtoll((char *) val, NULL, 10);
	intval = intval + increment;

	/* The max value for an unsigned long long is 18446744073709551615,
	 * and strlen('18446744073709551615') = 20, so if the value is smaller
	 * than 24 (just in case) we create a new buffer. */
	if (vsize < 24) {
		unsigned char *nv = malloc(24);
		if (nv == NULL)
			return -3;
		free(val);
		e->val = val = nv;
		e->vsize = vsize = 24;
	}

	snprintf((char *) val, vsize, "%23lld", (long long int) intval);
	*newval = intval;

	return 0;
}


