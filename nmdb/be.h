
#ifndef _BE_H
#define _BE_H

/* The following should be specific to the db backend we use. As we only
 * handle qdbm for now, there's no need to play with #ifdefs. */
#include <depot.h>
typedef DEPOT db_t;


db_t *db_open(const char *name, int flags);
int db_close(db_t *db);
int db_set(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int db_get(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int db_del(db_t *db, const unsigned char *key, size_t ksize);

#endif


