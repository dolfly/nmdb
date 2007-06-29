
#include "nmdb.h"
#include <ruby.h>

static VALUE rb_mnmdb_ll;
static VALUE rb_cDB;


/* Functions for ruby memory management.
 * The "mm" prefix is used by us (not enforced by Ruby) to distinguish it from
 * the rest. */

static void mm_db_mark(nmdb_t *db)
{
}

static void mm_db_free(nmdb_t *db)
{
	nmdb_free(db);
}

static VALUE mm_db_allocate(VALUE class)
{
	nmdb_t *db = nmdb_init();

	return Data_Wrap_Struct(class, mm_db_mark, mm_db_free, db);
}


/*
 * DB methods
 */

VALUE m_add_tipc_server(VALUE self, VALUE port)
{
	nmdb_t *db;
	Data_Get_Struct(self, nmdb_t, db);

	int rv = nmdb_add_tipc_server(db, NUM2INT(port));
	return INT2NUM(rv);
}

VALUE m_add_tcp_server(VALUE self, VALUE hostname, VALUE port)
{
	nmdb_t *db;
	Data_Get_Struct(self, nmdb_t, db);

	int rv = nmdb_add_tcp_server(db, StringValuePtr(hostname),
			NUM2INT(port));
	return INT2NUM(rv);
}

VALUE m_add_udp_server(VALUE self, VALUE hostname, VALUE port)
{
	nmdb_t *db;
	Data_Get_Struct(self, nmdb_t, db);

	int rv = nmdb_add_udp_server(db, StringValuePtr(hostname),
			NUM2INT(port));
	return INT2NUM(rv);
}


/* Set functions */
typedef int (*setf_t) (nmdb_t *db,
		const unsigned char *k, size_t ks,
		const unsigned char *v, size_t vs);
static VALUE generic_set(VALUE self, VALUE key, VALUE val, setf_t set_func)
{
	int rv;
	unsigned char *k, *v;
	size_t ksize, vsize;
	nmdb_t *db;
	Data_Get_Struct(self, nmdb_t, db);

	k = rb_str2cstr(key, &ksize);
	v = rb_str2cstr(val, &vsize);

	rv = set_func(db, k, ksize, v, vsize);
	return INT2NUM(rv);
}

static VALUE m_set(VALUE self, VALUE key, VALUE val) {
	return generic_set(self, key, val, nmdb_set);
}

static VALUE m_set_sync(VALUE self, VALUE key, VALUE val) {
	return generic_set(self, key, val, nmdb_set_sync);
}

static VALUE m_cache_set(VALUE self, VALUE key, VALUE val) {
	return generic_set(self, key, val, nmdb_cache_set);
}


/* Get functions */
typedef ssize_t (*getf_t) (nmdb_t *db,
		const unsigned char *k, size_t ks,
		unsigned char *v, size_t vs);
VALUE generic_get(VALUE self, VALUE key, getf_t get_func)
{
	ssize_t rv;
	unsigned char *k, *v;
	size_t ksize, vsize;
	VALUE retval;
	nmdb_t *db;
	Data_Get_Struct(self, nmdb_t, db);

	k = rb_str2cstr(key, &ksize);

	v = ALLOC_N(char, (68 * 1024));

	rv = get_func(db, k, ksize, v, (68 * 1024));
	if (rv >= 0)
		retval = rb_str_new(v, rv);
	else
		/* The high level module knows and checks for this, there's no
		 * need to make the C code more complex by raising an
		 * exception. */
		retval = INT2NUM(rv);

	free(v);
	return retval;
}

VALUE m_get(VALUE self, VALUE key) {
	return generic_get(self, key, nmdb_get);
}

VALUE m_cache_get(VALUE self, VALUE key) {
	return generic_get(self, key, nmdb_cache_get);
}


/* Del functions */
typedef int (*delf_t) (nmdb_t *db, const unsigned char *k, size_t ks);
VALUE generic_del(VALUE self, VALUE key, delf_t del_func)
{
	ssize_t rv;
	unsigned char *k;
	size_t ksize;
	nmdb_t *db;
	Data_Get_Struct(self, nmdb_t, db);

	k = rb_str2cstr(key, &ksize);

	rv = del_func(db, k, ksize);
	return INT2NUM(rv);
}

VALUE m_del(VALUE self, VALUE key) {
	return generic_del(self, key, nmdb_del);
}

VALUE m_del_sync(VALUE self, VALUE key) {
	return generic_del(self, key, nmdb_del_sync);
}

VALUE m_cache_del(VALUE self, VALUE key) {
	return generic_del(self, key, nmdb_cache_del);
}


/* CAS functions */
typedef int (*casf_t) (nmdb_t *db,
		const unsigned char *k, size_t ks,
		const unsigned char *ov, size_t ovs,
		const unsigned char *nv, size_t nvs);
static VALUE generic_cas(VALUE self, VALUE key, VALUE oldval, VALUE newval,
		casf_t cas_func)
{
	int rv;
	unsigned char *k, *ov, *nv;
	size_t ksize, ovsize, nvsize;
	nmdb_t *db;
	Data_Get_Struct(self, nmdb_t, db);

	k = rb_str2cstr(key, &ksize);
	ov = rb_str2cstr(oldval, &ovsize);
	nv = rb_str2cstr(newval, &nvsize);

	rv = cas_func(db, k, ksize, ov, ovsize, nv, nvsize);
	return INT2NUM(rv);
}

static VALUE m_cas(VALUE self, VALUE key, VALUE oldval, VALUE newval) {
	return generic_cas(self, key, oldval, newval, nmdb_cas);
}

static VALUE m_cache_cas(VALUE self, VALUE key, VALUE oldval, VALUE newval) {
	return generic_cas(self, key, oldval, newval, nmdb_cache_cas);
}


/* Module initialization */
void Init_nmdb_ll()
{
	rb_mnmdb_ll = rb_define_module("Nmdb_ll");
	rb_cDB = rb_define_class_under(rb_mnmdb_ll, "DB", rb_cObject);
	rb_define_alloc_func(rb_cDB, mm_db_allocate);

	rb_define_method(rb_cDB, "add_tipc_server", m_add_tipc_server, 1);
	rb_define_method(rb_cDB, "add_tcp_server", m_add_tcp_server, 2);
	rb_define_method(rb_cDB, "add_udp_server", m_add_udp_server, 2);

	rb_define_method(rb_cDB, "set", m_set, 2);
	rb_define_method(rb_cDB, "set_sync", m_set_sync, 2);
	rb_define_method(rb_cDB, "cache_set", m_cache_set, 2);

	rb_define_method(rb_cDB, "get", m_get, 1);
	rb_define_method(rb_cDB, "cache_get", m_cache_get, 1);

	rb_define_method(rb_cDB, "del", m_del, 1);
	rb_define_method(rb_cDB, "del_sync", m_del_sync, 1);
	rb_define_method(rb_cDB, "cache_del", m_cache_del, 1);

	rb_define_method(rb_cDB, "cas", m_cas, 3);
	rb_define_method(rb_cDB, "cache_cas", m_cache_cas, 3);
}

