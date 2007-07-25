
#include <stddef.h>	/* size_t */
#include "be.h"


db_t *db_open(const char *name, int flags)
{
	/* Use a dumb not-null pointer because it is never looked at outside
	 * the functions defined here */
	return (db_t *) 1;
}


int db_close(db_t *db)
{
	return 1;
}


int db_set(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	return 1;
}


int db_get(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	return 0;
}

int db_del(db_t *db, const unsigned char *key, size_t ksize)
{
	return 0;
}

