
#if BE_ENABLE_NULL

#include <stddef.h>	/* size_t */
#include <stdlib.h>	/* malloc() and friends */
#include "be.h"


int null_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int null_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int null_del(struct db_conn *db, const unsigned char *key, size_t ksize);
int null_close(struct db_conn *db);


struct db_conn *null_open(const char *name, int flags)
{
	struct db_conn *db;

	db = malloc(sizeof(struct db_conn));
	if (db == NULL)
		return NULL;

	db->conn = NULL;
	db->set = null_set;
	db->get = null_get;
	db->del = null_del;
	db->close = null_close;

	return db;
}


int null_close(struct db_conn *db)
{
	free(db);
	return 1;
}


int null_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return 1;
}


int null_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	return 0;
}

int null_del(struct db_conn *db, const unsigned char *key, size_t ksize)
{
	return 0;
}

#else

#include <stddef.h>	/* NULL */

struct db_conn *null_open(const char *name, int flags)
{
	return NULL;
}

#endif

