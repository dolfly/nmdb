
#ifndef _HASH_H
#define _HASH_H

/*
 * Hash function used in the cache.
 *
 * This is kept here instead of a .c file so the compiler can inline if it
 * decides it's worth it.
 *
 * The hash function used is Austin Appleby's MurmurHash2. It used to be
 * Jenkins's one-at-a-time, but this one is much faster. However, we keep
 * the others that were tested for future comparison purposes.
 */
#define hash(k, s) murmurhash2(k, s)

/* MurmurHash2, by Austin Appleby. The one we use.
 * It has been modify to fit into the coding style, to work on uint32_t
 * instead of ints, and the seed was fixed to a random number because it's not
 * an issue for us. The author placed it in the public domain, so it's safe.
 * http://sites.google.com/site/murmurhash/ */
static uint32_t murmurhash2(const unsigned char *key, size_t len)
{
	const uint32_t m = 0x5bd1e995;
	const int r = 24;
	const uint32_t seed = 0x34a4b627;

	// Initialize the hash to a 'random' value
	uint32_t h = seed ^ len;

	// Mix 4 bytes at a time into the hash
	while (len >= 4) {
		uint32_t k = *(uint32_t *) key;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		key += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array
	switch (len) {
		case 3: h ^= key[2] << 16;
		case 2: h ^= key[1] << 8;
		case 1: h ^= key[0];
			h *= m;
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}


/* Unused functions, left for comparison purposes */
#if 0

/* Paul Hsieh's SuperFastHash, which is really fast, but a tad slower than
 * MurmurHash2.
 * http://www.azillionmonkeys.com/qed/hash.html */
static uint32_t superfast(const unsigned char *data, size_t len)
{
	uint32_t h = len, tmp;
	int rem;

	/* this is the same as (*((const uint16_t *) (d))) on little endians,
	 * but we keep the generic version since gcc compiles decent code for
	 * both and there's no noticeable difference in performance */
	#define get16bits(d) ( \
		( ( (uint32_t) ( ((const uint8_t *)(d))[1] ) ) << 8 ) \
		+ (uint32_t) ( ((const uint8_t *)(d))[0] ) )

	if (len <= 0 || data == NULL)
		return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (; len > 0; len--) {
		h += get16bits(data);
		tmp = (get16bits(data+2) << 11) ^ h;
		h = (h << 16) ^ tmp;
		data += 2*sizeof (uint16_t);
		h += h >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3: h += get16bits(data);
			h ^= h << 16;
			h ^= data[sizeof(uint16_t)] << 18;
			h += h >> 11;
			break;
		case 2: h += get16bits(data);
			h ^= h << 11;
			h += h >> 17;
			break;
		case 1: h += *data;
			h ^= h << 10;
			h += h >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	h ^= h << 3;
	h += h >> 5;
	h ^= h << 4;
	h += h >> 17;
	h ^= h << 25;
	h += h >> 6;

	return h;

	#undef get16bits
}

/* Jenkins' one-at-a-time hash, the one we used to use.
 * http://en.wikipedia.org/wiki/Jenkins_hash_function */
static uint32_t oneatatime(const unsigned char *key, const size_t ksize)
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

/* FNV 32 bit hash; slower than the rest.
 * http://www.isthe.com/chongo/tech/comp/fnv/ */
static uint32_t fnv_hash(const unsigned char *key, const size_t ksize)
{
	const unsigned char *end = key + ksize;
	uint32_t hval;

	hval = 0x811c9dc5;
	while (key < end) {
		hval += (hval<<1) + (hval<<4) + (hval<<7) +
			(hval<<8) + (hval<<24);
		hval ^= (uint32_t) *key++;
	}

	return hval;
}

#endif

#endif

