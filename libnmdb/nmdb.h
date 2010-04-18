
/* Header for the libnmdb library. */

#ifndef _NMDB_H
#define _NMDB_H

/** Opaque type representing a connection with one or more servers. */
typedef struct nmdb_conn nmdb_t;

/** Create a new nmdb_t pointer, used to talk with the server.
 *
 * @returns a new connection instance.
 * @ingroup connection
 */
nmdb_t *nmdb_init();

/** Add a TIPC server.
 *
 * @param db connection instance.
 * @param port TIPC port to connect to (-1 means use the default).
 * @returns 1 on success, 0 on failure.
 * @ingroup connection
 */
int nmdb_add_tipc_server(nmdb_t *db, int port);

/** Add a TCP server.
 *
 * @param db connection instance.
 * @param addr host to connect to.
 * @param port port to connect to (-1 means use the default).
 * @returns 1 on success, 0 on failure.
 * @ingroup connection
 */
int nmdb_add_tcp_server(nmdb_t *db, const char *addr, int port);

/** Add an UDP server.
 *
 * @param db connection instance.
 * @param addr host to connect to.
 * @param port port to connect to (-1 means use the default).
 * @returns 1 on success, 0 on failure.
 * @ingroup connection
 */
int nmdb_add_udp_server(nmdb_t *db, const char *addr, int port);

/** Add a SCTP server.
 *
 * @param db connection instance.
 * @param addr host to connect to.
 * @param port port to connect to (-1 means use the default).
 * @returns 1 on success, 0 on failure.
 * @ingroup connection
 */
int nmdb_add_sctp_server(nmdb_t *db, const char *addr, int port);

/** Free a nmdb_t structure created by nmdb_init().
 * It also closes all the connections opened to the servers.
 *
 * @param db connection instance.
 * @returns 1 on success, 0 on failure.
 * @ingroup connection
 */
int nmdb_free(nmdb_t *db);

/** Get the value associated with a key.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param[out] val buffer where the value will be stored.
 * @param vsize size of the value buffer.
 * @returns the size of the value written to the given buffer, -1 if the key
 * 	is not in the database, or -2 if there was an error.
 * @ingroup database
 */
ssize_t nmdb_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);

/** Get the value associated with a key from cache.
 * This is just like nmdb_get(), except it only queries the caches, and never
 * the database.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param[out] val buffer where the value will be stored.
 * @param vsize size of the value buffer.
 * @returns the size of the value written to the given buffer, -1 if the key
 * 	is not in the cache, or -2 if there was an error.
 * @ingroup cache
 */
ssize_t nmdb_cache_get(nmdb_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);

/** Set the value associated with a key.
 * It returns after the command has been acknowledged by the server, but does
 * not wait for the database to confirm it. In any case, further GET requests
 * will return this value, even if this set has not reached the backend yet.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param val the value
 * @param vsize size of the value.
 * @returns 1 on success, < 0 on error.
 * @ingroup database
 */
int nmdb_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);

/** Set the value associated with a key, synchronously.
 * It works just like nmdb_set(), except it returns only after the database
 * confirms it has stored the value.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param val the value
 * @param vsize size of the value.
 * @returns 1 on success, < 0 on error.
 * @ingroup database
 */
int nmdb_set_sync(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);

/** Set the value associated with a key, but only in the cache.
 * This command sets the key's value in the cache, but does NOT affect the
 * backend database. Combining it with nmdb_set() and/or nmdb_set_sync() is
 * not recommended, because consistency issues may arise.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param val the value
 * @param vsize size of the value.
 * @returns 1 on success, < 0 on error.
 * @ingroup cache
 */
int nmdb_cache_set(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *val, size_t vsize);

/** Delete a key.
 * It returns after the command has been acknowledged by the server, but does
 * not wait for the database to confirm it. In any case, further GET requests
 * will return the key is missing, even if this delete has not reached the
 * backend yet.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @returns 1 on success, < 0 on error.
 * @ingroup database
 */
int nmdb_del(nmdb_t *db, const unsigned char *key, size_t ksize);

/** Delete a key synchronously.
 * It works just like nmdb_del(), except it returns only after the database
 * confirms it has deleted the key.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @returns 1 on success, < 0 on error.
 * @ingroup database
 */
int nmdb_del_sync(nmdb_t *db, const unsigned char *key, size_t ksize);

/** Delete a key, but only from the cache.
 * This command deletes a key from the cache, but does NOT affect the backend
 * database. Combining it with nmdb_del() and/or nmdb_del_sync() is not
 * recommended, because consistency issues may arise.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @returns 1 on success, < 0 on error.
 * @ingroup cache
 */
int nmdb_cache_del(nmdb_t *db, const unsigned char *key, size_t ksize);

/** Perform an atomic compare-and-swap.
 * This command will set the value associated to a key to newval, provided it
 * currently is oldval, in an atomic way.
 * Equivalent to atomically doing:
 *
 * 	if get(key) == oldval, then set(key, newval)
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param oldval the old, expected value.
 * @param ovsize size of the expected value.
 * @param newval the new value to set.
 * @param nvsize size of the new value.
 * @returns 2 on success, 1 if the current value does not match oldval, 0 if
 * 	the key is not in the database, or < 0 on error.
 * @ingroup database
 */
int nmdb_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize);

/** Perform an atomic compare-and-swap only on the cache.
 * This command works just like nmdb_cas(), except it affects only the cache,
 * and not the backend database.
 * Equivalent to atomically doing:
 *
 * 	if get_cache(key) == oldval, then set_cache(key, newval)
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param oldval the old, expected value.
 * @param ovsize size of the expected value.
 * @param newval the new value to set.
 * @param nvsize size of the new value.
 * @returns 2 on success, 1 if the current value does not match oldval, 0 if
 * 	the key is not in the database, or < 0 on error.
 * @ingroup cache
 */
int nmdb_cache_cas(nmdb_t *db, const unsigned char *key, size_t ksize,
		const unsigned char *oldval, size_t ovsize,
		const unsigned char *newval, size_t nvsize);

/** Atomically increment the value associated with a key.
 * This command atomically increments the value associated with a key in the
 * given increment. However, there are requirements on the current value: it
 * must be a NULL-terminated string with only a number in base 10 in it (i.e.
 * it must be parseable by strtoll(3)).
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param increment the value to add to the current one (can be negative).
 * @param[out] newval pointer to an integer that will be set to the new
 * 	value.
 * @returns 2 if the increment was successful, 1 if the current value was not
 * 	in an appropriate format, 0 if the key is not in the database, or < 0
 * 	on error.
 * @ingroup database
 */
int nmdb_incr(nmdb_t *db, const unsigned char *key, size_t ksize,
		int64_t increment, int64_t *newval);

/** Atomically increment the value associated with a key only in the cache.
 * This command works just like nmdb_incr(), except it affects only the cache,
 * and not the backend database.
 *
 * @param db connection instance.
 * @param key the key.
 * @param ksize the key size.
 * @param increment the value to add to the current one (can be negative).
 * @param[out] newval pointer to an integer that will be set to the new
 * 	value.
 * @returns 2 if the increment was successful, 1 if the current value was not
 * 	in an appropriate format, 0 if the key is not in the database, or < 0
 * 	on error.
 * @ingroup cache
 */
int nmdb_cache_incr(nmdb_t *db, const unsigned char *key, size_t ksize,
                int64_t increment, int64_t *newval);


/** Request servers' statistics.
 * This API is used by nmdb-stats, and likely to change in the future. Do not
 * rely on it.
 *
 * @param db connection instance.
 * @param[out] buf buffer used to store the results.
 * @param bsize size of the buffer.
 * @param[out] nservers number of servers queried.
 * @param [out] nstats number of stats per server.
 * @returns 1 if success, -1 if there was an error in the server, -2 if there
 *	was a network error, -3 if the buffer was too small, -4 if the server
 *	replies were of different size (indicates different server versions,
 *	not supported at the time)
 */
int nmdb_stats(nmdb_t *db, unsigned char *buf, size_t bsize,
		unsigned int *nservers, unsigned int *nstats);

#endif

