
#if BE_ENABLE_QDBM

#include <depot.h>	/* QDBM's Depot API */
#include <stdlib.h>

#include "be.h"


int qdbm_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int qdbm_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int qdbm_del(struct db_conn *db, const unsigned char *key, size_t ksize);
int qdbm_close(struct db_conn *db);


struct db_conn *qdbm_open(const char *name, int flags)
{
	int f;
	struct db_conn *db;
	DEPOT *qdbm_db;

	f = DP_OREADER | DP_OWRITER | DP_ONOLCK | DP_OCREAT;
	qdbm_db = dpopen(name, f, 0);
	if (qdbm_db == NULL)
		return NULL;

	db = malloc(sizeof(struct db_conn));
	if (db == NULL) {
		dpclose(qdbm_db);
		return NULL;
	}

	db->conn = qdbm_db;
	db->set = qdbm_set;
	db->get = qdbm_get;
	db->del = qdbm_del;
	db->close = qdbm_close;

	return db;
}


int qdbm_close(struct db_conn *db)
{
	int rv;

	rv = dpclose(db->conn);
	free(db);
	return rv;
}


int qdbm_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return dpput(db->conn, (char *) key, ksize,
			(char *) val, vsize, DP_DOVER);
}


int qdbm_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	int rv;

	rv = dpgetwb(db->conn, (char *) key, ksize, 0, *vsize, (char *) val);
	if (rv >= 0) {
		*vsize = rv;
		return 1;
	} else {
		return 0;
	}
}

int qdbm_del(struct db_conn *db, const unsigned char *key, size_t ksize)
{
	return dpout(db->conn, (char *) key, ksize);
}

#else

#include <stddef.h>	/* NULL */

struct db_conn *qdbm_open(const char *name, int flags)
{
	return NULL;
}

#endif

