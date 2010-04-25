
#if BE_ENABLE_TDB

#include <string.h>	/* memcpy() */
#include <stdlib.h>	/* malloc() and friends */

/* tdb.h needs mode_t defined externally, and it is defined in one of these
(which are the ones required for open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tdb.h>

#include "be.h"

/* Local operations names are prepended with an 'x' so they don't collide with
 * tdb real functions */

int xtdb_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int xtdb_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int xtdb_del(struct db_conn *db, const unsigned char *key, size_t ksize);
int xtdb_close(struct db_conn *db);


struct db_conn *xtdb_open(const char *name, int flags)
{
	struct db_conn *db;
	TDB_CONTEXT *tdb_db;

	tdb_db = tdb_open(name, 0, 0, O_CREAT | O_RDWR, 0640);
	if (tdb_db == NULL)
		return NULL;

	db = malloc(sizeof(struct db_conn));
	if (db == NULL) {
		tdb_close(tdb_db);
		return NULL;
	}

	db->conn = tdb_db;
	db->set = xtdb_set;
	db->get = xtdb_get;
	db->del = xtdb_del;
	db->close = xtdb_close;

	return db;
}

int xtdb_close(struct db_conn *db)
{
	int rv;

	rv = tdb_close(db->conn);
	free(db);
	return rv == 0;
}

int xtdb_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	TDB_DATA k, v;

	/* we can't maintain "const"ness here because tdb's prototypes; the
	 * same applies to get and del, so we just cast */
	k.dptr = (unsigned char *) key;
	k.dsize = ksize;
	v.dptr = (unsigned char *) val;
	v.dsize = vsize;

	return tdb_store(db->conn, k, v, TDB_REPLACE) == 0;
}

int xtdb_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	TDB_DATA k, v;

	k.dptr = (unsigned char *) key;
	k.dsize = ksize;

	v = tdb_fetch(db->conn, k);
	if (v.dptr == NULL)
		return 0;

	if (v.dsize > *vsize)
		return 0;

	*vsize = v.dsize;
	memcpy(val, v.dptr, v.dsize);
	free(v.dptr);
	return 1;
}

int xtdb_del(struct db_conn *db, const unsigned char *key, size_t ksize)
{
	TDB_DATA k;

	k.dptr = (unsigned char *) key;
	k.dsize = ksize;

	return tdb_delete(db->conn, k) == 0;
}

#else

#include <stddef.h>	/* NULL */

struct db_conn *xtdb_open(const char *name, int flags)
{
	return NULL;
}

#endif

