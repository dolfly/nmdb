
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
>>> db.add_tipc_server()
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

try:
	import cPickle as pickle
except ImportError:
	import pickle

import nmdb_ll


class NetworkError (Exception):
	pass


class GenericDB (object):
	def __init__(self):
		self._db = nmdb_ll.new()
		self.autopickle = True

	def add_tipc_server(self, port = -1):
		"Adds a TIPC server to the server pool."
		rv = self._db.add_tipc_server(port)
		if not rv:
			raise NetworkError
		return rv

	def add_tcp_server(self, addr, port = -1):
		"Adds a TCP server to the server pool."
		rv = self._db.add_tcp_server(addr, port)
		if not rv:
			raise NetworkError
		return rv

	def add_udp_server(self, addr, port = -1):
		"Adds an UDP server to the server pool."
		rv = self._db.add_udp_server(addr, port)
		if not rv:
			raise NetworkError
		return rv


	def generic_get(self, getf, key):
		"d[k]   Returns the value associated with the key k."
		if self.autopickle:
			key = pickle.dumps(key, protocol = -1)
		try:
			r = getf(key)
		except:
			raise NetworkError
		if r == -1:
			# For key errors, get returns -1 instead of a string
			# so we know it's a miss.
			raise KeyError
		if self.autopickle:
			r = pickle.loads(r)
		return r

	def cache_get(self, key):
		return self.generic_get(self._db.cache_get, key)

	def normal_get(self, key):
		return self.generic_get(self._db.get, key)


	def generic_set(self, setf, key, val):
		"d[k] = v   Associates the value v to the key k."
		if self.autopickle:
			key = pickle.dumps(key, protocol = -1)
			val = pickle.dumps(val, protocol = -1)
		r = setf(key, val)
		if r <= 0:
			raise NetworkError
		return 1

	def cache_set(self, key, val):
		return self.generic_set(self._db.cache_set, key, val)

	def normal_set(self, key, val):
		return self.generic_set(self._db.set, key, val)

	def set_sync(self, key, val):
		return self.generic_set(self._db.set_sync, key, val)


	def generic_delete(self, delf, key):
		"del d[k]   Deletes the key k."
		if self.autopickle:
			key = pickle.dumps(key, protocol = -1)
		r = delf(key)
		if r < 0:
			raise NetworkError
		elif r == 0:
			raise KeyError
		return 1

	def cache_delete(self, key):
		return self.generic_delete(self._db.cache_delete, key)

	def normal_delete(self, key):
		return self.generic_delete(self._db.delete, key)

	def delete_sync(self, key):
		return self.generic_delete(self._db.delete_sync, key)


	def generic_cas(self, casf, key, oldval, newval):
		"Perform a compare-and-swap."
		if self.autopickle:
			key = pickle.dumps(key, protocol = -1)
			oldval = pickle.dumps(oldval, protocol = -1)
			newval = pickle.dumps(newval, protocol = -1)
		r = casf(key, oldval, newval)
		if r == 2:
			# success
			return 2
		elif r == 1:
			# no match
			return 1
		elif r == 0:
			# not in
			raise KeyError
		else:
			raise NetworkError

	def cache_cas(self, key, oldv, newv):
		return self.generic_cas(self._db.cache_cas, key,
				oldval, newval)

	def normal_cas(self, key, oldval, newval):
		return self.generic_cas(self._db.cas, key,
				oldval, newval)


	def generic_incr(self, incrf, key, increment):
		"""Atomically increment the value associated with the given
		key by the given increment."""
		if self.autopickle:
			key = pickle.dumps(key, protocol = -1)
		r, v = incrf(key, increment)
		if r == 2:
			# success
			return v
		elif r == 1:
			# no match, because the value didn't have the right
			# format
			raise TypeError(
				"The value must be a NULL-terminated string")
		elif r == 0:
			# not in
			raise KeyError
		else:
			raise NetworkError

	def cache_incr(self, key, increment = 1):
		return self.generic_incr(self._db.cache_incr, key, increment)

	def normal_incr(self, key, increment = 1):
		return self.generic_incr(self._db.incr, key, increment)

	def firstkey(self):
		try:
			r = self._db.firstkey()
		except:
			raise NetworkError
		if r == -1:
			# No keys, or unsupported
			raise KeyError
		if self.autopickle:
			r = pickle.loads(r)
		return r

	def nextkey(self, key):
		if self.autopickle:
			key = pickle.dumps(key, protocol = -1)
		try:
			r = self._db.nextkey(key)
		except:
			raise NetworkError
		if r == -1:
			# No next key, or unsupported
			raise KeyError
		if self.autopickle:
			r = pickle.loads(r)
		return r

	# The following functions will assume the existance of self.set,
	# self.get, and self.delete, which are supposed to be set by our
	# subclasses.

	def __getitem__(self, key):
		return self.get(key)

	def __setitem__(self, key, val):
		return self.set(key, val)

	def __delitem__(self, key):
		return self.delete(key)

	def __contains__(self, key):
		"Returns True if the key is in the database, False otherwise."
		try:
			r = self.get(key)
		except KeyError:
			return False
		if not r:
			return False
		return True

	def has_key(self, key):
		"Returns True if the key is in the database, False otherwise."
		return self.__contains__(key)



class Cache (GenericDB):
	get = GenericDB.cache_get
	set = GenericDB.cache_set
	delete = GenericDB.cache_delete
	cas = GenericDB.cache_cas
	incr = GenericDB.cache_incr

class DB (GenericDB):
	get = GenericDB.normal_get
	set = GenericDB.normal_set
	delete = GenericDB.normal_delete
	cas = GenericDB.normal_cas
	incr = GenericDB.normal_incr
	firstkey = GenericDB.firstkey
	nextkey = GenericDB.nextkey

class SyncDB (GenericDB):
	get = GenericDB.normal_get
	set = GenericDB.set_sync
	delete = GenericDB.delete_sync
	cas = GenericDB.normal_cas
	incr = GenericDB.normal_incr
	firstkey = GenericDB.firstkey
	nextkey = GenericDB.nextkey


