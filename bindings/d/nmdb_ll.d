
/* Low-level bindings, used by nmdb.d. */

module nmdb_ll;


/* We need to define the nmdb_t type used by the C API.
 *
 * One possiblity is to import the C struct here (alignment and stuff is
 * supposed to be the same), but since it includes some OS structures like
 * sockaddr_tipc, it means several lines of code and it's difficult to
 * maintain. Because to us it's an opaque type, we define it to be an ubyte
 * array of the same length as C's struct nmdb_t. That way we retain ABI
 * compatibility but minimize the clutter.
 *
 * To port this to another architecture, just compile and run the "sizeof.c"
 * program. It should output some lines like "sizeof(struct nmdb_t) = 16".
 * Then use that information to define the aliases for your platform.
 *
 * Should nmdb_t change, the numbers must be updated to reflect the new sizes.
 */

version (X86) {
	/* Generated on a Pentium II running Ubuntu. It should be the same on
	 * all x86 Linux boxes. */
	alias ubyte[12] nmdb_t;
}

version (X86_64) {
	/* Generated on a Pentium D running Gentoo in 64 bits mode. It should
	 * be the same on all Linux amd64 boxes. */
	alias ubyte[16] nmdb_t;
}


/* nmdb structures and prototypes, these shouldn't need any changes
 * unless libnmdb/nmdb.h is updated */

extern (C) nmdb_t *nmdb_init(int port);
extern (C) int nmdb_add_server(nmdb_t *db, int port);
extern (C) int nmdb_free(nmdb_t *db);

extern (C) ptrdiff_t nmdb_get(nmdb_t *db, ubyte *key, size_t ksize,
		ubyte *val, size_t vsize);
extern (C) ptrdiff_t nmdb_cache_get(nmdb_t *db, ubyte *key, size_t ksize,
		ubyte *val, size_t vsize);

extern (C) int nmdb_set(nmdb_t *db, ubyte *key, size_t ksize,
		ubyte *val, size_t vsize);
extern (C) int nmdb_set_sync(nmdb_t *db, ubyte *key, size_t ksize,
		ubyte *val, size_t vsize);
extern (C) int nmdb_cache_set(nmdb_t *db, ubyte *key, size_t ksize,
		ubyte *val, size_t vsize);

extern (C) int nmdb_del(nmdb_t *db, ubyte *key, size_t ksize);
extern (C) int nmdb_del_sync(nmdb_t *db, ubyte *key, size_t ksize);
extern (C) int nmdb_cache_del(nmdb_t *db, ubyte *key, size_t ksize);

extern (C) int nmdb_cas(nmdb_t *db, ubyte *key, size_t ksize,
		ubyte *oldval, size_t ovsize, ubyte *newval, size_t nvsize);
extern (C) int nmdb_cache_cas(nmdb_t *db, ubyte *key, size_t ksize,
		ubyte *oldval, size_t ovsize, ubyte *newval, size_t nvsize);

