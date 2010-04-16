
#include <string.h>	/* memcpy() */

/* tdb.h needs mode_t defined externally, and it is defined in one of these
(which are the ones required for open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "be.h"


db_t *db_open(const char *name, int flags)
{
	return tdb_open(name, 0, 0, O_CREAT | O_RDWR, 0640);
}


int db_close(db_t *db)
{
	return tdb_close(db) == 0;
}


int db_set(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	TDB_DATA k, v;

	/* we can't maintain "const"ness here because tdb's prototypes; the
	 * same applies to get and del */
	k.dptr = key;
	k.dsize = ksize;
	v.dptr = val;
	v.dsize = vsize;

	return tdb_store(db, k, v, TDB_REPLACE) == 0;
}


int db_get(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	TDB_DATA k, v;

	k.dptr = key;
	k.dsize = ksize;

	v = tdb_fetch(db, k);
	if (v.dptr == NULL)
		return 0;

	if (v.dsize > *vsize)
		return 0;

	*vsize = v.dsize;
	memcpy(val, v.dptr, v.dsize);
	free(v.dptr);
	return 1;
}

int db_del(db_t *db, const unsigned char *key, size_t ksize)
{
	TDB_DATA k;

	k.dptr = key;
	k.dsize = ksize;

	return tdb_delete(db, k) == 0;
}

