
/*
 * Digital Mars D programming language bindings for the nmdb C library.
 * Alberto Bertogli (albertito@gmail.com)
 */

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


	this()
	{
		db = nmdb_init();
	}

	~this()
	{
		nmdb_free(db);
	}

	void add_tipc_server(int port = -1)
	{
		int r = nmdb_add_tipc_server(db, port);
		if (r == 0) {
			throw new Exception("Can't add server");
		}
	}

	void add_tcp_server(char[] addr, int port = -1)
	{
		int r = nmdb_add_tcp_server(db, cast(ubyte *) addr.ptr, port);
		if (r == 0) {
			throw new Exception("Can't add server");
		}
	}

	void add_udp_server(char[] addr, int port = -1)
	{
		int r = nmdb_add_udp_server(db, cast(ubyte *) addr.ptr, port);
		if (r == 0) {
			throw new Exception("Can't add server");
		}
	}

	void add_sctp_server(char[] addr, int port = -1)
	{
		int r = nmdb_add_sctp_server(db, cast(ubyte *) addr.ptr, port);
		if (r == 0) {
			throw new Exception("Can't add server");
		}
	}

	private char[] do_get(char[] key, int mode)
	{
		ptrdiff_t size;
		ubyte* k = cast(ubyte *) key.ptr;
		auto v = new char[64 * 1024];

		if (mode == MODE_NORMAL || mode == MODE_SYNC) {
			size = nmdb_get(db, k, key.length,
					cast(ubyte *) v, v.length);
		} else if (mode == MODE_CACHE) {
			size = nmdb_cache_get(db, k, key.length,
					cast(ubyte *) v, v.length);
		} else {
			throw new Exception("Invalid mode");
		}

		if (size == -1) {
			throw new KeyNotFound("Key not found: " ~ key);
		} else if (size <= -2) {
			throw new Exception("Can't get value");
		}

		// resize using D's magic
		v.length = cast(size_t) size;

		return v;
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

	private int do_cas(char[] key, char[] oldval, char[] newval,
			int mode)
	{
		ubyte* k = cast(ubyte *) key.ptr;
		ubyte* ov = cast(ubyte *) oldval.ptr;
		ubyte* nv = cast(ubyte *) newval.ptr;
		int res = 0;

		if (mode == MODE_NORMAL || mode == MODE_SYNC) {
			res = nmdb_cas(db, k, key.length,
					ov, oldval.length,
					nv, newval.length);
		} else if (mode == MODE_CACHE) {
			res = nmdb_cache_cas(db, k, key.length,
					ov, oldval.length,
					nv, newval.length);
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


	int cas(char[] key, char[] oldval, char[] newval)
	{
		return do_cas(key, oldval, newval, mode);
	}

	int cas_normal(char[] key, char[] oldval, char[] newval)
	{
		return do_cas(key, oldval, newval, MODE_NORMAL);
	}

	int cache_cas(char[] key, char[] oldval, char[] newval)
	{
		return do_cas(key, oldval, newval, MODE_CACHE);
	}


	char[] opIndex(char[] key)
	{
		return get(key);
	}

	void opIndexAssign(char[] val, char[] key)
	{
		return set(key, val);
	}

	bool opIn_r(char[] key)
	{
		try {
			get(key);
		} catch (KeyNotFound(s)) {
			return false;
		}
		return true;
	}

}

