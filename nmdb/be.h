
#ifndef _BE_H
#define _BE_H

#include <stddef.h>	/* size_t */

struct db_conn {
	/* This is where the backend sets a reference to the connection, which
	 * will be properly casted when needed */
	void *conn;

	/* Operations */
	int (*set)(struct db_conn *db, const unsigned char *key, size_t ksize,
			unsigned char *val, size_t vsize);
	int (*get)(struct db_conn *db, const unsigned char *key, size_t ksize,
			unsigned char *val, size_t *vsize);
	int (*del)(struct db_conn *db, const unsigned char *key, size_t ksize);
	int (*firstkey)(struct db_conn *db, unsigned char *key, size_t *ksize);
	int (*nextkey)(struct db_conn *db,
			const unsigned char *key, size_t ksize,
			unsigned char *nextkey, size_t *nksize);
	int (*close)(struct db_conn *db);
};

enum backend_type {
	/* The first two are special, used to indicate unknown and unsupported
	 * backends */
	BE_UNKNOWN = -2,
	BE_UNSUPPORTED = -1,
	BE_QDBM = 1,
	BE_BDB,
	BE_TC,
	BE_TDB,
	BE_NULL,
};

/* Generic opener that knows about all the backends */
struct db_conn *db_open(enum backend_type type, const char *name, int flags);

/* Returns the backend type for the given name. */
enum backend_type be_type_from_str(const char *name);

/* Returns the backend name for the given type. */
const char *be_str_from_type(enum backend_type type);

/* String containing a list of all supported backends */
#if BE_ENABLE_QDBM
  #define _QDBM_SUPP "qdbm "
#else
  #define _QDBM_SUPP ""
#endif

#if BE_ENABLE_BDB
  #define _BDB_SUPP "bdb "
#else
  #define _BDB_SUPP ""
#endif

#if BE_ENABLE_TC
  #define _TC_SUPP "tc "
#else
  #define _TC_SUPP ""
#endif

#if BE_ENABLE_TDB
  #define _TDB_SUPP "tdb "
#else
  #define _TDB_SUPP ""
#endif

#if BE_ENABLE_NULL
  #define _NULL_SUPP "null "
#else
  #define _NULL_SUPP ""
#endif

#define SUPPORTED_BE _QDBM_SUPP _BDB_SUPP _TC_SUPP _TDB_SUPP _NULL_SUPP


/* Default backend */
#if BE_ENABLE_TDB
  #define DEFAULT_BE BE_TDB
  #define DEFAULT_BE_NAME "tdb"
#elif BE_ENABLE_TC
  #define DEFAULT_BE BE_TC
  #define DEFAULT_BE_NAME "tc"
#elif BE_ENABLE_QDBM
  #define DEFAULT_BE BE_QDBM
  #define DEFAULT_BE_NAME "qdbm"
#elif BE_ENABLE_BDB
  #define DEFAULT_BE BE_BDB
  #define DEFAULT_BE_NAME "bdb"
#elif BE_ENABLE_NULL
  #warning "using null backend as the default"
  #define DEFAULT_BE BE_NULL
  #define DEFAULT_BE_NAME "null"
#else
  #error "no backend available"
#endif


#endif

