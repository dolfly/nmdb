
#include <depot.h>	/* QDBM's Depot API */
#include <stdlib.h>

#include "be.h"


db_t *db_open(const char *name, int flags)
{
	int f;

	f = DP_OREADER | DP_OWRITER | DP_ONOLCK | DP_OCREAT;
	return dpopen(name, f, 0);
}


int db_close(db_t *db)
{
	return dpclose(db);
}


int db_set(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return dpput(db, (char *) key, ksize, (char *) val, vsize, DP_DOVER);
}


int db_get(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	int rv;

	rv = dpgetwb(db, (char *) key, ksize, 0, *vsize, (char *) val);
	if (rv >= 0) {
		*vsize = rv;
		return 1;
	} else {
		return 0;
	}
}

int db_del(db_t *db, const unsigned char *key, size_t ksize)
{
	return dpout(db, (char *) key, ksize);
}

