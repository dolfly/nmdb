
/*
 * Python 2 and 3 bindings for libnmdb
 * Alberto Bertogli (albertito@blitiri.com.ar)
 *
 * This is the low-level module, used by the python one to construct
 * friendlier objects.
 */

#define PY_SSIZE_T_CLEAN 1

#include <Python.h>
#include <nmdb.h>


/*
 * Type definitions
 */

typedef struct {
	PyObject_HEAD;
	nmdb_t *db;
} nmdbobject;

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
	size_t ksize, vsize;
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
	size_t ksize, vsize;
	ssize_t rv;
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
#ifdef PYTHON3
		r = PyBytes_FromStringAndSize((char *) val, rv);
#elif PYTHON2
		r = PyString_FromStringAndSize((char *) val, rv);
#endif
	}

	free(val);
	return r;
}

/* cache delete */
static PyObject *db_cache_delete(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	size_t ksize;
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
	size_t ksize, ovsize, nvsize;
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
	size_t ksize;
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

	return Py_BuildValue("iL", rv, newval);
}


/* db set */
static PyObject *db_set(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *val;
	size_t ksize, vsize;
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
	size_t ksize, vsize;
	ssize_t rv;
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
#ifdef PYTHON3
		r = PyBytes_FromStringAndSize((char *) val, rv);
#elif PYTHON2
		r = PyString_FromStringAndSize((char *) val, rv);
#endif
	}

	free(val);
	return r;
}

/* db delete */
static PyObject *db_delete(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	size_t ksize;
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
	size_t ksize, ovsize, nvsize;
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
	size_t ksize;
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
	size_t ksize, vsize;
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
	size_t ksize;
	int rv;

	if (!PyArg_ParseTuple(args, "s#:delete_sync", &key, &ksize)) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_del_sync(db->db, key, ksize);
	Py_END_ALLOW_THREADS

	return PyLong_FromLong(rv);
}

/* firstkey */
static PyObject *db_firstkey(nmdbobject *db, PyObject *args)
{
	unsigned char *key;
	size_t ksize;
	ssize_t rv;
	PyObject *r;

	if (!PyArg_ParseTuple(args, "")) {
		return NULL;
	}

	/* ksize is enough to hold the any value */
	ksize = 128 * 1024;
	key = malloc(ksize);
	if (key== NULL)
		return PyErr_NoMemory();

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_firstkey(db->db, key, ksize);
	Py_END_ALLOW_THREADS

	if (rv <= -2) {
		/* FIXME: define a better exception */
		r = PyErr_SetFromErrno(PyExc_IOError);
	} else if (rv == -1) {
		/* No first key or unsupported, handled in the high-level
		 * module. */
		r = PyLong_FromLong(-1);
	} else {
#ifdef PYTHON3
		r = PyBytes_FromStringAndSize((char *) key, rv);
#elif PYTHON2
		r = PyString_FromStringAndSize((char *) key, rv);
#endif
	}

	free(key);
	return r;
}

/* nextkey */
static PyObject *db_nextkey(nmdbobject *db, PyObject *args)
{
	unsigned char *key, *newkey;
	size_t ksize, nksize;
	ssize_t rv;
	PyObject *r;

	if (!PyArg_ParseTuple(args, "s#:nextkey", &key, &ksize)) {
		return NULL;
	}

	/* nksize is enough to hold the any value */
	nksize = 128 * 1024;
	newkey = malloc(nksize);
	if (newkey == NULL)
		return PyErr_NoMemory();

	Py_BEGIN_ALLOW_THREADS
	rv = nmdb_nextkey(db->db, key, ksize, newkey, nksize);
	Py_END_ALLOW_THREADS

	if (rv <= -2) {
		/* FIXME: define a better exception */
		r = PyErr_SetFromErrno(PyExc_IOError);
	} else if (rv == -1) {
		/* End, handled in the high-level module. */
		r = PyLong_FromLong(-1);
	} else {
#ifdef PYTHON3
		r = PyBytes_FromStringAndSize((char *) newkey, rv);
#elif PYTHON2
		r = PyString_FromStringAndSize((char *) newkey, rv);
#endif
	}

	free(newkey);
	return r;
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
	{ "firstkey", (PyCFunction) db_firstkey, METH_VARARGS, NULL },
	{ "nextkey", (PyCFunction) db_nextkey, METH_VARARGS, NULL },

	{ NULL }
};



/*
 * The module
 */

/* new, returns an nmdb object */
static PyTypeObject nmdbType;
static PyObject *db_new(PyObject *self, PyObject *args)
{
	nmdbobject *db;

#ifdef PYTHON3
	db = (nmdbobject *) nmdbType.tp_alloc(&nmdbType, 0);
#elif PYTHON2
	db = PyObject_New(nmdbobject, &nmdbType);
#endif
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

static PyMethodDef nmdb_functions[] = {
	{ "new", (PyCFunction) db_new, METH_VARARGS, NULL },
	{ NULL }
};


#ifdef PYTHON3
static PyTypeObject nmdbType = {
	PyObject_HEAD_INIT(NULL)
	.tp_name = "nmdb_ll.nmdb",
	.tp_itemsize = sizeof(nmdbobject),
	.tp_dealloc = (destructor) db_dealloc,
	.tp_methods = nmdb_methods,
};

static PyModuleDef nmdb_module = {
	PyModuleDef_HEAD_INIT,
	.m_name = "nmdb_ll",
	.m_doc = NULL,
	.m_size = -1,
	.m_methods = nmdb_functions,
};

PyMODINIT_FUNC PyInit_nmdb_ll(void)
{
	PyObject *m;

	if (PyType_Ready(&nmdbType) < 0)
		return NULL;

	m = PyModule_Create(&nmdb_module);

	return m;
}

#elif PYTHON2
static PyObject *db_getattr(nmdbobject *db, char *name)
{
	return Py_FindMethod(nmdb_methods, (PyObject *)db, name);
}

static PyTypeObject nmdbType = {
	PyObject_HEAD_INIT(NULL)
	0,
	"nmdb_ll.nmdb",
	sizeof(nmdbobject),
	0,
	(destructor) db_dealloc,
	0,
	(getattrfunc) db_getattr,
};

PyMODINIT_FUNC initnmdb_ll(void)
{
	nmdbType.ob_type = &PyType_Type;

	Py_InitModule("nmdb_ll", nmdb_functions);
}

#endif


