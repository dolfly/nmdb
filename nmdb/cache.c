
/* Generic cache layer.
 * It's a hash table with cache-style properties, keeping a (non-precise) size
 * and using a natural, per-chain LRU to do cleanups.
 * Cleanups are performed in place, when cache_set() gets called.
 */

#include <sys/types.h>		/* for size_t */
#include <stdint.h>		/* for uint32_t */
#include <stdlib.h>		/* for malloc() */
#include <string.h>		/* for memcpy()/memcmp() */
#include "cache.h"

struct cache *cache_create(size_t numobjs, unsigned int flags)
{
	size_t i;
	struct cache *cd;
	struct cache_chain *c;

	cd = (struct cache *) malloc(sizeof(struct cache));
	if (cd == NULL)
		return NULL;

	cd->flags = flags;

	/* We calculate the hash size so we have 4 objects per bucket; 4 being
	 * an arbitrary number. It's long enough to make LRU useful, and small
	 * enough to make lookups fast. */
	cd->chainlen = 4;
	cd->hashlen = numobjs / cd->chainlen;

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
			free(e);
			e = n;
		}
	}

	free(cd->table);
	free(cd);
	return 1;
}


/*
 * The hash function used is the "One at a time" function, which seems simple,
 * fast and popular. Others for future consideration if speed becomes an issue
 * include:
 *  * FNV Hash (http://www.isthe.com/chongo/tech/comp/fnv/)
 *  * SuperFastHash (http://www.azillionmonkeys.com/qed/hash.html)
 *  * Judy dynamic arrays (http://judy.sf.net)
 *
 * A good comparison can be found at
 * http://eternallyconfuzzled.com/tuts/hashing.html#existing
 */

static uint32_t hash(const unsigned char *key, const size_t ksize)
{
	uint32_t h = 0;
	size_t i;

	for (i = 0; i < ksize; i++) {
		h += key[i];
		h += (h << 10);
		h ^= (h >> 6);
	}
	h += (h << 3);
	h ^= (h >> 11);
	h += (h << 15);
	return h;
}


/*
 * Looks given key up in the chain.
 * Returns NULL if not found, or a pointer to the cache entry if it's found.
 * The chain can be empty.
 * Used in cache_get() and cache_set().
 */
static struct cache_entry *find_in_chain(struct cache_chain *c,
		const unsigned char *key, size_t ksize)
{
	int found = 0;
	struct cache_entry *e;

	e = c->first;
	while (e != NULL) {
		if (ksize != e->ksize) {
			e = e->next;
			continue;
		}
		if (memcmp(key, e->key, ksize) == 0) {
			found = 1;
			break;
		}

		e = e->next;
	}

	if (found)
		return e;
	return NULL;

}

/*
 * Gets the matching value for the given key.
 * Returns 0 if no match was found, or 1 otherwise.
 */
int cache_get(struct cache *cd, const unsigned char *key, size_t ksize,
		unsigned char **val, size_t *vsize)
{
	uint32_t h = 0;
	struct cache_chain *c;
	struct cache_entry *e;

	h = hash(key, ksize) % cd->hashlen;
	c = cd->table + h;

	e = find_in_chain(c, key, ksize);

	if (e == NULL) {
		*val = NULL;
		*vsize = 0;
		return 0;
	}

	*val = e->val;
	*vsize = e->vsize;

	return 1;
}


int cache_set(struct cache *cd, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize)
{
	int rv = 1;
	uint32_t h = 0;
	struct cache_chain *c;
	struct cache_entry *e, *new;
	unsigned char *v;

	h = hash(key, ksize) % cd->hashlen;
	c = cd->table + h;

	e = find_in_chain(c, key, ksize);

	if (e == NULL) {
		/* not found, create a new cache entry */
		new = malloc(sizeof(struct cache_entry));
		if (new == NULL) {
			rv = 0;
			goto exit;
		}

		new->ksize = ksize;
		new->vsize = vsize;

		new->key = malloc(ksize);
		if (new->key == NULL) {
			free(new);
			rv = 0;
			goto exit;
		}
		memcpy(new->key, key, ksize);

		new->val = malloc(vsize);
		if (new->val == NULL) {
			free(new->key);
			free(new);
			rv = 0;
			goto exit;
		}
		memcpy(new->val, val, vsize);
		new->prev = NULL;
		new->next = NULL;

		/* and put it in */
		if (c->len == 0) {
			/* line is empty, just put it there */
			c->first = new;
			c->last = new;
			c->len = 1;
		} else if (c->len <= cd->chainlen) {
			/* slots are still available, put the entry first */
			new->next = c->first;
			c->first->prev = new;
			c->first = new;
			c->len += 1;
		} else {
			/* chain is full, we need to evict the last one */
			e = c->last;
			c->last = e->prev;
			c->last->next = NULL;
			free(e->key);
			free(e->val);
			free(e);

			new->next = c->first;
			c->first->prev = new;
			c->first = new;
		}
	} else {
		/* we've got a match, just replace the value in place */
		v = malloc(vsize);
		if (v == NULL) {
			rv = 0;
			goto exit;
		}
		free(e->val);
		e->val = v;
		memcpy(e->val, val, vsize);

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

exit:
	return rv;
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
	free(e);

	c->len -= 1;

exit:
	return rv;
}

