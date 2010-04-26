
#if BE_ENABLE_TC

#include <tchdb.h>	/* Tokyo Cabinet's hash API */
#include <stdlib.h>

#include "be.h"


int tc_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int tc_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int tc_del(struct db_conn *db, const unsigned char *key, size_t ksize);
int tc_close(struct db_conn *db);


struct db_conn *tc_open(const char *name, int flags)
{
	struct db_conn *db;
	TCHDB *tc_db = tchdbnew();

	if (!tchdbopen(tc_db, name, HDBOWRITER | HDBOCREAT))
		return NULL;

	db = malloc(sizeof(struct db_conn));
	if (db == NULL) {
		tchdbclose(tc_db);
		tchdbdel(tc_db);
		return NULL;
	}

	db->conn = tc_db;
	db->set = tc_set;
	db->get = tc_get;
	db->del = tc_del;
	db->firstkey = NULL;
	db->nextkey = NULL;
	db->close = tc_close;

	return db;
}


int tc_close(struct db_conn *db)
{
	int r = tchdbclose(db->conn);
	tchdbdel(db->conn);
	free(db);
	return r;
}


int tc_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return tchdbput(db->conn, key, ksize, val, vsize);
}


int tc_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	int rv;

	rv = tchdbget3(db->conn, key, ksize, val, *vsize);
	if (rv >= 0) {
		*vsize = rv;
		return 1;
	} else {
		return 0;
	}
}

int tc_del(struct db_conn *db, const unsigned char *key, size_t ksize)
{
	return tchdbout(db->conn, key, ksize);
}

#else

#include <stddef.h>	/* NULL */

struct db_conn *tc_open(const char *name, int flags)
{
	return NULL;
}

#endif

