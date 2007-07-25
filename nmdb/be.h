
#ifndef _BE_H
#define _BE_H

/* Depending on our backend, we define db_t to be different things so the
 * generic code doesn't have to care about which backend we're using. */
#if defined BACKEND_QDBM
  #include <depot.h>
  typedef DEPOT db_t;
#elif defined BACKEND_NULL
  typedef int db_t;
#else
  #error "Unknown backend"
  /* Define it anyway, so this is the only warning/error the user sees. */
  typedef int db_t;
#endif


db_t *db_open(const char *name, int flags);
int db_close(db_t *db);
int db_set(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int db_get(db_t *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int db_del(db_t *db, const unsigned char *key, size_t ksize);

#endif


