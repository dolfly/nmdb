
module nmdb;

import nmdb_ll;


/* Exception to raise when a key can't be found. It'd be better to use the
 * same D arrays use (ArrayBoundsException) but it's designed for compiler
 * use, and the lack of a standard exception hierarchy sucks. */
class KeyNotFound : Exception {
	this(char[] s) {
		super(s);
	}
}


/* Operation modes */
enum {
	MODE_NORMAL = 1,
	MODE_CACHE = 2,
	MODE_SYNC = 3,
}


class DB
{
	private nmdb_t *db;
	int mode = MODE_NORMAL;


	this(int port = 10)
	{
		db = nmdb_init(port);
	}

	~this()
	{
		nmdb_free(db);
	}


	private char[] do_get(char[] key, int mode)
	{
		long size;
		ubyte* k = cast(ubyte *) key.ptr;
		auto v = new char[256];

		if (mode == MODE_NORMAL || mode == MODE_SYNC) {
			size = nmdb_get(db, k, key.length,
					cast(ubyte *) v, v.sizeof);
		} else if (mode == MODE_CACHE) {
			size = nmdb_cache_get(db, k, key.length,
					cast(ubyte *) v, v.sizeof);
		} else {
			throw new Exception("Invalid mode");
		}

		if (size == 0) {
			throw new KeyNotFound("Key not found: " ~ key);
		} else if (size < 0) {
			throw new Exception("Can't get value");
		}

		return v[0 .. cast(ulong) size];
	}

	private void do_set(char[] key, char[] val, int mode)
	{
		ubyte* k = cast(ubyte *) key.ptr;
		ubyte* v = cast(ubyte *) val.ptr;
		int res = 0;

		if (mode == MODE_NORMAL) {
			res = nmdb_set(db, k, key.length, v, val.length);
		} else if (mode == MODE_SYNC) {
			res = nmdb_set_sync(db, k, key.length,
					v, val.length);
		} else if (mode == MODE_CACHE) {
			res = nmdb_cache_set(db, k, key.length,
					v, val.length);
		} else {
			throw new Exception("Invalid mode");
		}

		if (res != 1) {
			throw new Exception("Can't set value");
		}
	}

	private int do_del(char[] key, int mode)
	{
		ubyte* k = cast(ubyte *) key.ptr;
		int res = 0;

		if (mode == MODE_NORMAL) {
			res = nmdb_del(db, k, key.length);
		} else if (mode == MODE_SYNC) {
			res = nmdb_del_sync(db, k, key.length);
		} else if (mode == MODE_CACHE) {
			res = nmdb_cache_del(db, k, key.length);
		} else {
			throw new Exception("Invalid mode");
		}
		return res;
	}


	char[] get(char[] key)
	{
		return do_get(key, mode);
	}

	char[] get_normal(char[] key)
	{
		return do_get(key, MODE_NORMAL);
	}

	char[] cache_get(char[] key)
	{
		return do_get(key, MODE_CACHE);
	}


	void set(char[] key, char[] val)
	{
		return do_set(key, val, mode);
	}

	void set_normal(char[] key, char[] val)
	{
		return do_set(key, val, MODE_NORMAL);
	}

	void set_sync(char[] key, char[] val)
	{
		return do_set(key, val, MODE_SYNC);
	}

	void cache_set(char[] key, char[] val)
	{
		return do_set(key, val, MODE_CACHE);
	}


	int remove(char[] key)
	{
		return do_del(key, mode);
	}

	int remove_normal(char[] key)
	{
		return do_del(key, MODE_NORMAL);
	}

	int remove_sync(char[] key)
	{
		return do_del(key, MODE_SYNC);
	}

	int cache_remove(char[] key)
	{
		return do_del(key, MODE_CACHE);
	}


	char[] opIndex(char[] key)
	{
		return get(key);
	}

	void opIndexAssign(char[] val, char[] key)
	{
		return set(key, val);
	}

}

