
#if BE_ENABLE_LEVELDB

#include <leveldb/c.h>	/* LevelDB C API */
#include <string.h>	/* memcpy() */
#include <stdlib.h>

#include "be.h"


int xleveldb_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize);
int xleveldb_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize);
int xleveldb_del(struct db_conn *db, const unsigned char *key, size_t ksize);
int xleveldb_close(struct db_conn *db);
int xleveldb_firstkey(struct db_conn *db, unsigned char *key, size_t *ksize);
int xleveldb_nextkey(struct db_conn *db,
		const unsigned char *key, size_t ksize,
		unsigned char *nextkey, size_t *nksize);

struct db_conn *xleveldb_open(const char *name, int flags)
{
	struct db_conn *db;
	leveldb_options_t *options;
	leveldb_t *level_db;

	options = leveldb_options_create();
	if (options == NULL)
		return NULL;

	leveldb_options_set_create_if_missing(options, 1);

	level_db = leveldb_open(options, name, NULL);

	leveldb_options_destroy(options);

	if (level_db == NULL)
		return NULL;

	db = malloc(sizeof(struct db_conn));
	if (db == NULL) {
		leveldb_close(level_db);
		return NULL;
	}

	db->conn = level_db;
	db->set = xleveldb_set;
	db->get = xleveldb_get;
	db->del = xleveldb_del;
	db->firstkey = xleveldb_firstkey;
	db->nextkey = xleveldb_nextkey;
	db->close = xleveldb_close;

	return db;
}


int xleveldb_close(struct db_conn *db)
{
	leveldb_close(db->conn);
	free(db);
	return 1;
}


int xleveldb_set(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t vsize)
{
	leveldb_writeoptions_t *options = leveldb_writeoptions_create();
	char *err, *origerr;

	err = origerr = malloc(1);
	leveldb_put(db->conn, options,
			(const char *) key, ksize,
			(const char *) val, vsize, &err);
	free(err);

	leveldb_writeoptions_destroy(options);

	return err == origerr;
}


int xleveldb_get(struct db_conn *db, const unsigned char *key, size_t ksize,
		unsigned char *val, size_t *vsize)
{
	int rv;
	char *db_val = NULL;
	size_t db_vsize;
	leveldb_readoptions_t *options = leveldb_readoptions_create();
	char *err, *origerr;

	err = origerr = malloc(1);
	db_val = leveldb_get(db->conn, options,
			(const char *) key, ksize, &db_vsize, &err);

	free(err);
	leveldb_readoptions_destroy(options);

	if (err != origerr || db_val == NULL || db_vsize > *vsize) {
		rv = 0;
		goto exit;
	}

	*vsize = db_vsize;
	memcpy(val, db_val, db_vsize);
	rv = 1;

exit:
	free(db_val);
	return rv;
}


int xleveldb_del(struct db_conn *db, const unsigned char *key, size_t ksize)
{
	leveldb_writeoptions_t *options = leveldb_writeoptions_create();
	char *err, *origerr;

	err = origerr = malloc(1);
	leveldb_delete(db->conn, options,
			(const char *) key, ksize, &err);
	free(err);

	leveldb_writeoptions_destroy(options);

	return err == origerr;
}


int xleveldb_firstkey(struct db_conn *db, unsigned char *key, size_t *ksize)
{
	int rv = 0;
	const char *db_key;
	size_t db_ksize;

	leveldb_readoptions_t *options = leveldb_readoptions_create();
	leveldb_iterator_t *it = leveldb_create_iterator(db->conn, options);

	leveldb_iter_seek_to_first(it);
	if (! leveldb_iter_valid(it)) {
		rv = 0;
		goto exit;
	}

	db_key = leveldb_iter_key(it, &db_ksize);

	if (db_key == NULL || db_ksize > *ksize) {
		rv = 0;
		goto exit;
	}

	*ksize = db_ksize;
	memcpy(key, db_key, db_ksize);
	rv = 1;

exit:
	leveldb_readoptions_destroy(options);
	leveldb_iter_destroy(it);
	return rv;
}


int xleveldb_nextkey(struct db_conn *db,
		const unsigned char *key, size_t ksize,
		unsigned char *nextkey, size_t *nksize)
{
	int rv;
	const char *db_nextkey;
	size_t db_nksize = 0;

	leveldb_readoptions_t *options = leveldb_readoptions_create();
	leveldb_iterator_t *it = leveldb_create_iterator(db->conn, options);

	leveldb_iter_seek(it, (const char *) key, ksize);
	if (! leveldb_iter_valid(it)) {
		rv = 0;
		goto exit;
	}

	leveldb_iter_next(it);
	if (! leveldb_iter_valid(it)) {
		rv = 0;
		goto exit;
	}

	db_nextkey = leveldb_iter_key(it, &db_nksize);

	if (db_nextkey == NULL || db_nksize > *nksize) {
		rv = 0;
		goto exit;
	}

	*nksize = db_nksize;
	memcpy(nextkey, db_nextkey, db_nksize);
	rv = 1;

exit:
	leveldb_iter_destroy(it);
	leveldb_readoptions_destroy(options);
	return rv;
}

#else

#include <stddef.h>	/* NULL */

struct db_conn *leveldb_open(const char *name, int flags)
{
	return NULL;
}

#endif

