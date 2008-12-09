
/*
 * Python 3 bindings for libnmdb
 * Alberto Bertogli (albertito@blitiri.com.ar)
 *
 * This is the low-level module, used by the python one to construct
 * friendlier objects.
 */

#include <Python.h>
#include <nmdb.h>


/*
 * Type definitions
 */

typedef struct {
	PyObject_HEAD;
	nmdb_t *db;
} nmdbobject;
static PyTypeObject nmdbType;

/*
 * The nmdb object
 */

/* delete */
static void db_dealloc(nmdbobject *db)
{
	if (db->db) {
		nmdb_free(db->db);
	}
	PyObject_Del(db);
}


/* add tipc server */
static PyObject *db_add_tipc_server(nmdbobject *db, PyObject *args)
{
	int port;
	int rv;

	if (!PyArg_ParseTuple(args, "i:add_tipc_server", &port)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_add_tipc_server(db->db, port);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* add tcp server */
static PyObject *db_add_tcp_server(nmdbobject *db, PyObject *args)
{
	int port;
	char *addr;
	int rv;

	if (!PyArg_ParseTuple(args, "si:add_tcp_server", &addr, &port)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_add_tcp_server(db->db, addr, port);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* add udp server */
static PyObject *db_add_udp_server(nmdbobject *db, PyObject *args)
{
	int port;
	char *addr;
	int rv;

	if (!PyArg_ParseTuple(args, "si:add_udp_server", &addr, &port)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_add_udp_server(db->db, addr, port);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* cache set */
static PyObject *db_cache_set(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *val;
	int ksize, vsize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#s#:cache_set", &key, &ksize,
				&val, &vsize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_cache_set(db->db, key, ksize, val, vsize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* cache get */
static PyObject *db_cache_get(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *val;
	int ksize, vsize;
	long rv;
	PyObject *r;

	if (!PyArg_ParseTuple(args, "s#:cache_get", &key, &ksize)) {
		return NULL;
	}

	/* vsize is enough to hold the any value */
	vsize = 128 * 1024;
	val = malloc(vsize);
	if (val == NULL)
		return PyErr_NoMemory();

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_cache_get(db->db, key, ksize, val, vsize);
	Py_END_ALLOW_THREADS

	if (rv <= -2) {
		/* FIXME: define a better exception */
		r = PyErr_SetFromErrno(PyExc_IOError);
	} else if (rv == -1) {
		/* Miss, handled in the high-level module. */
		r = PyLong_FromLong(-1);
	} else {
		r = PyBytes_FromStringAndSize((char *) val, rv);
	}

	free(val);
	return r;
}

/* cache delete */
static PyObject *db_cache_delete(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	int ksize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#:cache_delete", &key, &ksize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_cache_del(db->db, key, ksize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* cache cas */
static PyObject *db_cache_cas(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *oldval, *newval;
	int ksize, ovsize, nvsize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#s#s#:cache_cas", &key, &ksize,
				&oldval, &ovsize,
				&newval, &nvsize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_cache_cas(db->db, key, ksize, oldval, ovsize,
			newval, nvsize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* cache increment */
static PyObject *db_cache_incr(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	int ksize;
	int rv;
	long long int increment;
	int64_t newval;

	if (!PyArg_ParseTuple(args, "s#L:cache_incr", &key, &ksize,
				&increment)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_cache_incr(db->db, key, ksize, increment, &newval);
	Py_END_ALLOW_THREADS

	return Py_BuildValue("LL", rv, newval);
}


/* db set */
static PyObject *db_set(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *val;
	int ksize, vsize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#s#:set", &key, &ksize,
				&val, &vsize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_set(db->db, key, ksize, val, vsize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* db get */
static PyObject *db_get(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *val;
	int ksize, vsize;
	long rv;
	PyObject *r;

	if (!PyArg_ParseTuple(args, "s#:get", &key, &ksize)) {
		return NULL;
	}

	/* vsize is enough to hold the any value */
	vsize = 128 * 1024;
	val = malloc(vsize);
	if (val == NULL)
		return PyErr_NoMemory();

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_get(db->db, key, ksize, val, vsize);
	Py_END_ALLOW_THREADS

	if (rv <= -2) {
		/* FIXME: define a better exception */
		r = PyErr_SetFromErrno(PyExc_IOError);
	} else if (rv == -1) {
		/* Miss, handled in the high-level module. */
		r = PyLong_FromLong(-1);
	} else {
		r = PyBytes_FromStringAndSize((char *) val, rv);
	}

	free(val);
	return r;
}

/* db delete */
static PyObject *db_delete(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	int ksize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#:delete", &key, &ksize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_del(db->db, key, ksize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* db cas */
static PyObject *db_cas(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *oldval, *newval;
	int ksize, ovsize, nvsize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#s#s#:cas", &key, &ksize,
				&oldval, &ovsize,
				&newval, &nvsize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_cas(db->db, key, ksize, oldval, ovsize, newval, nvsize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* db increment */
static PyObject *db_incr(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	int ksize;
	int rv;
	long long int increment;
	int64_t newval;

	if (!PyArg_ParseTuple(args, "s#L:incr", &key, &ksize, &increment)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_incr(db->db, key, ksize, increment, &newval);
	Py_END_ALLOW_THREADS

	return Py_BuildValue("LL", rv, newval);
}


/* db set sync */
static PyObject *db_set_sync(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *val;
	int ksize, vsize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#s#:set_sync", &key, &ksize,
				&val, &vsize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_set_sync(db->db, key, ksize, val, vsize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* db delete sync */
static PyObject *db_delete_sync(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	int ksize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#:delete_sync", &key, &ksize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_del_sync(db->db, key, ksize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}



/* nmdb method table */

static PyMethodDef nmdb_methods[] = {
	{ "add_tipc_server", (PyCFunction) db_add_tipc_server,
		METH_VARARGS, NULL },
	{ "add_tcp_server", (PyCFunction) db_add_tcp_server,
		METH_VARARGS, NULL },
	{ "add_udp_server", (PyCFunction) db_add_udp_server,
		METH_VARARGS, NULL },
	{ "cache_set", (PyCFunction) db_cache_set, METH_VARARGS, NULL },
	{ "cache_get", (PyCFunction) db_cache_get, METH_VARARGS, NULL },
	{ "cache_delete", (PyCFunction) db_cache_delete, METH_VARARGS, NULL },
	{ "cache_cas", (PyCFunction) db_cache_cas, METH_VARARGS, NULL },
	{ "cache_incr", (PyCFunction) db_cache_incr, METH_VARARGS, NULL },
	{ "set", (PyCFunction) db_set, METH_VARARGS, NULL },
	{ "get", (PyCFunction) db_get, METH_VARARGS, NULL },
	{ "delete", (PyCFunction) db_delete, METH_VARARGS, NULL },
	{ "cas", (PyCFunction) db_cas, METH_VARARGS, NULL },
	{ "incr", (PyCFunction) db_incr, METH_VARARGS, NULL },
	{ "set_sync", (PyCFunction) db_set_sync, METH_VARARGS, NULL },
	{ "delete_sync", (PyCFunction) db_delete_sync, METH_VARARGS, NULL },

	{ NULL }
};

/* new, returns an nmdb object */
static PyObject *db_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	nmdbobject *db;

	db = (nmdbobject *) type->tp_alloc(type, 0);
	if (db == NULL)
		return NULL;

	if (!PyArg_ParseTuple(args, ":new")) {
		return NULL;
	}

	db->db = nmdb_init();
	if (db->db == NULL) {
		return PyErr_NoMemory();
	}

	/* XXX: is this necessary? */
	if (PyErr_Occurred()) {
		nmdb_free(db->db);
		return NULL;
	}

	return (PyObject *) db;
}


static PyTypeObject nmdbType = {
	PyObject_HEAD_INIT(NULL)
	.tp_name = "nmdb_ll.nmdb",
	.tp_itemsize = sizeof(nmdbobject),
	.tp_dealloc = (destructor) db_dealloc,
	.tp_methods = nmdb_methods,
	.tp_new = db_new,
};



/*
 * The module
 */

static PyModuleDef nmdb_module = {
	PyModuleDef_HEAD_INIT,
	.m_name = "nmdb_ll",
	.m_doc = NULL,
	.m_size = -1,
};


PyMODINIT_FUNC PyInit_nmdb_ll(void)
{
	PyObject *m;

	if (PyType_Ready(&nmdbType) < 0)
		return NULL;

	m = PyModule_Create(&nmdb_module);

	Py_INCREF(&nmdbType);
	PyModule_AddObject(m, "nmdb", (PyObject *) &nmdbType);

	return m;
}



