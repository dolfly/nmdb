
#include <string.h>		/* strcmp() */
#include "be.h"

/* Openers for each supported backend, defined on each be-X.c file */
struct db_conn *qdbm_open(const char *name, int flags);
struct db_conn *bdb_open(const char *name, int flags);
struct db_conn *tc_open(const char *name, int flags);
struct db_conn *xtdb_open(const char *name, int flags);
struct db_conn *null_open(const char *name, int flags);


struct db_conn *db_open(enum backend_type type, const char *name, int flags)
{
	switch (type) {
	case BE_QDBM:
		return qdbm_open(name, flags);
	case BE_BDB:
		return bdb_open(name, flags);
	case BE_TC:
		return tc_open(name, flags);
	case BE_TDB:
		return xtdb_open(name, flags);
	case BE_NULL:
		return null_open(name, flags);
	default:
		return NULL;
	}
}

enum backend_type be_type_from_str(const char *name)
{
	if (strcmp(name, "qdbm") == 0)
		return BE_ENABLE_QDBM ? BE_QDBM : BE_UNSUPPORTED;
	if (strcmp(name, "bdb") == 0)
		return BE_ENABLE_BDB ? BE_BDB : BE_UNSUPPORTED;
	if (strcmp(name, "tc") == 0)
		return BE_ENABLE_TC ? BE_TC : BE_UNSUPPORTED;
	if (strcmp(name, "tdb") == 0)
		return BE_ENABLE_TDB ? BE_TDB : BE_UNSUPPORTED;
	if (strcmp(name, "null") == 0)
		return BE_ENABLE_NULL ? BE_NULL : BE_UNSUPPORTED;
	return BE_UNKNOWN;
}


const char *be_str_from_type(enum backend_type type)
{
	if (type == BE_QDBM)
		return "qdbm";
	if (type == BE_BDB)
		return "bdb";
	if (type == BE_TC)
		return "tc";
	if (type == BE_TDB)
		return "tdb";
	if (type == BE_NULL)
		return "null";
	return "unknown";
}

