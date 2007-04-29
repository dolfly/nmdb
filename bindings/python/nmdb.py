
"""
libnmdb python wrapper

This module is a wrapper for the libnmdb, the C library used to implement
clients to the nmdb server.

It provides three similar classes: DB, SyncDB and Cache. They all present the
same dictionary-alike interface, but differ in how they interact with the
server.

The DB class allows you to set, get and delete (key, value) pairs from the
database; the SyncDB class works like DB, but does so in a synchronous way; and
the Cache class affects only the cache and do not impact the backend database.

Note that mixing cache sets with DB sets can create inconsistencies between
the database and the cache. You shouldn't do that unless you know what you're
doing.

The classes use pickle to allow you to store and retrieve python objects in a
transparent way. To disable it, set .autopickle to False.

Here is an example using the DB class:

>>> import nmdb
>>> db = nmdb.DB()
>>> import nmdb
>>> db = nmdb.DB()
>>> db[1] = { 'english': 'one', 'castellano': 'uno', 'quechua': 'huk' }
>>> print db[1]
{'english': 'one', 'castellano': 'uno', 'quechua': 'huk'}
>>> db[(1, 2)] = (True, False)
>>> print db[(1, 2)]
(True, False)
>>> del db[(1, 2)]
>>> print db[(1, 2)]
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
  File "/usr/lib64/python2.5/site-packages/nmdb.py", line 74, in __getitem__
    raise KeyError
KeyError
"""

import cPickle
import nmdb_ll


class NetworkError (Exception):
	pass


class _nmdbDict (object):
	def __init__(self, db, op_get, op_set, op_delete, op_cas):
		self._db = db
		self._get = op_get
		self._set = op_set
		self._delete = op_delete
		self._cas = op_cas
		self.autopickle = True

	def add_server(self, port):
		"Adds a server to the server pool."
		rv = self._db.add_server(port)
		if not rv:
			raise NetworkError
		return rv

	def __getitem__(self, key):
		"d[k]   Returns the value associated with the key k."
		if self.autopickle:
			key = str(hash(key))
		try:
			r = self._get(key)
		except:
			raise NetworkError
		if not r:
			raise KeyError
		if self.autopickle:
			r = cPickle.loads(r)
		return r

	def __setitem__(self, key, val):
		"d[k] = v   Associates the value v to the key k."
		if self.autopickle:
			key = str(hash(key))
			val = cPickle.dumps(val, protocol = -1)
		r = self._set(key, val)
		if r <= 0:
			raise NetworkError
		return 1

	def __delitem__(self, key):
		"del d[k]   Deletes the key k."
		if self.autopickle:
			key = str(hash(key))
		r = self._delete(key)
		if r < 0:
			raise NetworkError
		elif r == 0:
			raise KeyError
		return 1

	def __contains__(self, key):
		"Returns True if the key is in the database, False otherwise."
		if self.autopickle:
			key = str(hash(key))
		try:
			r = self._get(key)
		except KeyError:
			return False
		if not r:
			return False
		return True

	def has_key(self, key):
		"Returns True if the key is in the database, False otherwise."
		return self.__contains__(key)

	def cas(self, key, oldval, newval):
		"Perform a compare-and-swap."
		if self.autopickle:
			key = str(hash(key))
			oldval = cPickle.dumps(oldval, protocol = -1)
			newval = cPickle.dumps(newval, protocol = -1)
		r = self._cas(key, oldval, newval)
		if r == 2:
			# success
			return 1
		elif r == 1:
			# no match
			return 0
		elif r == 0:
			# not in
			raise KeyError
		else:
			raise NetworkError


class Cache (_nmdbDict):
	def __init__(self, port = -1):
		db = nmdb_ll.connect(port)
		_nmdbDict.__init__(self, db, db.cache_get, db.cache_set,
					db.cache_delete, db.cache_cas)

class DB (_nmdbDict):
	def __init__(self, port = -1):
		db = nmdb_ll.connect(port)
		_nmdbDict.__init__(self, db, db.get, db.set, db.delete, db.cas)

class SyncDB (_nmdbDict):
	def __init__(self, port = -1):
		db = nmdb_ll.connect(port)
		_nmdbDict.__init__(self, db, db.get, db.set_sync,
					db.delete_sync, db.cas)


