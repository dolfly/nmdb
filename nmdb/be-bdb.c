
#if BE_ENABLE_BDB

#include <string.h>	/* memset() */
#include <stddef.h>	/* NULL */
#include <stdlib.h>	/* malloc() and friends */

/* typedefs to work around db.h include bug */
typedef unsigned int u_int;
typedef unsigned long u_long;
#include <db.h>

#include "be.h"


int bdb_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int bdb_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int bdb_del(struct db_conn *db, const unsigned char *key, size_t ksize);
int bdb_close(struct db_conn *db);


struct db_conn *bdb_open(const char *name, int flags)
{
	int rv;
	struct db_conn *db;
	DB *bdb_db;

	rv = db_create(&bdb_db, NULL, 0);
	if (rv != 0)
		return NULL;

	rv = bdb_db->open(bdb_db, NULL, name, NULL, DB_HASH, DB_CREATE, 0);
	if (rv != 0) {
		bdb_db->close(bdb_db, 0);
		return NULL;
	}

	db = malloc(sizeof(struct db_conn));
	if (db == NULL) {
		bdb_db->close(bdb_db, 0);
		return NULL;
	}

	db->conn = bdb_db;
	db->set = bdb_set;
	db->get = bdb_get;
	db->del = bdb_del;
	db->firstkey = NULL;
	db->nextkey = NULL;
	db->close = bdb_close;

	return db;
}


int bdb_close(struct db_conn *db)
{
	int rv;
	DB *bdb_db = (DB *) db->conn;

	rv = bdb_db->close(bdb_db, 0);
	if (rv != 0)
		return 0;

	free(db);
	return 1;
}


int bdb_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	int rv;
	DBT k, v;
	DB *bdb_db = (DB *) db->conn;

	memset(&k, 0, sizeof(DBT));
	memset(&v, 0, sizeof(DBT));

	/* we can't maintain "const"ness here because bdb's prototypes; the
	 * same applies to get and del, so we just cast */
	k.data = (unsigned char *) key;
	k.size = ksize;
	v.data = (unsigned char *) val;
	v.size = vsize;

	rv = bdb_db->put(bdb_db, NULL, &k, &v, 0);
	if (rv != 0)
		return 0;
	return 1;
}


int bdb_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	int rv;
	DBT k, v;
	DB *bdb_db = (DB *) db->conn;

	memset(&k, 0, sizeof(DBT));
	memset(&v, 0, sizeof(DBT));

	k.data = (unsigned char *) key;
	k.size = ksize;
	v.data = val;
	v.ulen = *vsize;
	v.flags = DB_DBT_USERMEM;	/* we supplied the memory */

	rv = bdb_db->get(bdb_db, NULL, &k, &v, 0);
	if (rv != 0) {
		return 0;
	} else {
		*vsize = v.size;
		return 1;
	}
}

int bdb_del(struct db_conn *db, const unsigned char *key, size_t ksize)
{
	int rv;
	DBT k, v;
	DB *bdb_db = (DB *) db->conn;

	memset(&k, 0, sizeof(DBT));
	memset(&v, 0, sizeof(DBT));

	k.data = (unsigned char *) key;
	k.size = ksize;

	rv = bdb_db->del(bdb_db, NULL, &k, 0);
	if (rv != 0)
		return 0;
	return 1;
}

#else

#include <stddef.h>	/* NULL */

struct db_conn *bdb_open(const char *name, int flags)
{
	return NULL;
}

#endif

