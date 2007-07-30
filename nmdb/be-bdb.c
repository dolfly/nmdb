
#include <string.h>	/* memset() */
#include "be.h"


db_t *db_open(const char *name, int flags)
{
	int rv;
	db_t *db;

	rv = db_create(&db, NULL, 0);
	if (rv != 0)
		return NULL;

	rv = db->open(db, NULL, name, NULL, DB_HASH, DB_CREATE, 0);
	if (rv != 0) {
		db->close(db, 0);
		return NULL;
	}

	return db;
}


int db_close(db_t *db)
{
	int rv;

	rv = db->close(db, 0);
	if (rv != 0)
		return 0;
	return 1;
}


int db_set(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	int rv;
	DBT k, v;

	memset(&k, 0, sizeof(DBT));
	memset(&v, 0, sizeof(DBT));

	/* we can't maintain "const"ness here because bdb's prototypes; the
	 * same applies to get and del */
	k.data = key;
	k.size = ksize;
	v.data = val;
	v.size = vsize;

	rv = db->put(db, NULL, &k, &v, 0);
	if (rv != 0)
		return 0;
	return 1;
}


int db_get(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	int rv;
	DBT k, v;

	memset(&k, 0, sizeof(DBT));
	memset(&v, 0, sizeof(DBT));

	k.data = key;
	k.size = ksize;
	v.data = val;
	v.ulen = *vsize;
	v.flags = DB_DBT_USERMEM;	/* we supplied the memory */

	rv = db->get(db, NULL, &k, &v, 0);
	if (rv != 0) {
		return 0;
	} else {
		*vsize = v.size;
		return 1;
	}
}

int db_del(db_t *db, const unsigned char *key, size_t ksize)
{
	int rv;
	DBT k, v;

	memset(&k, 0, sizeof(DBT));
	memset(&v, 0, sizeof(DBT));

	k.data = key;
	k.size = ksize;

	rv = db->del(db, NULL, &k, 0);
	if (rv != 0)
		return 0;
	return 1;
}

