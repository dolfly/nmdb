
#include <tchdb.h>	/* Tokyo Cabinet's hash API */
#include <stdlib.h>

#include "be.h"


db_t *db_open(const char *name, int flags)
{
	db_t *db = tchdbnew();

	if (!tchdbopen(db, name, HDBOWRITER | HDBOCREAT))
		return NULL;

	return db;
}


int db_close(db_t *db)
{
	int r = tchdbclose(db);
	tchdbdel(db);
	return r;
}


int db_set(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return tchdbput(db, key, ksize, val, vsize);
}


int db_get(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	int rv;

	rv = tchdbget3(db, key, ksize, val, *vsize);
	if (rv >= 0) {
		*vsize = rv;
		return 1;
	} else {
		return 0;
	}
}

int db_del(db_t *db, const unsigned char *key, size_t ksize)
{
	return tchdbout(db, key, ksize);
}

