
#
# libnmdb python wrapper
# Alberto Bertogli (albertito@gmail.com)
#

import nmdb_ll

class NetworkError (Exception):
	pass

class _nmdbDict (object):
	def __init__(self, db, op_get, op_set, op_delete):
		self.db = db
		self.op_get = op_get
		self.op_set = op_set
		self.op_delete = op_delete

	def __getitem__(self, key):
		try:
			r = self.op_get(key)
		except:
			raise NetworkError
		if not r:
			raise KeyError
		return r

	def __setitem__(self, key, val):
		r = self.op_set(key, val)
		if r <= 0:
			raise NetworkError
		return 1

	def __delitem__(self, key):
		r = self.op_delete(key)
		if r < 0:
			raise NetworkError
		elif r == 0:
			raise KeyError
		return 1

	def __contains__(self, key):
		try:
			r = self.op_get(key)
		except:
			raise NetworkError
		if not r:
			return False
		return True

	def has_key(self, key):
		return self.__contains__(key)


class Cache (_nmdbDict):
	def __init__(self, port = -1):
		db = nmdb_ll.connect(port)
		_nmdbDict.__init__(self, db, db.cache_get, db.cache_set,
					db.cache_delete)

class DB (_nmdbDict):
	def __init__(self, port = -1):
		db = nmdb_ll.connect(port)
		_nmdbDict.__init__(self, db, db.get, db.set, db.delete)


class AsyncDB (_nmdbDict):
	def __init__(self, port = -1):
		db = nmdb_ll.connect(port)
		_nmdbDict.__init__(self, db, db.get, db.set_async,
					db.delete_async)


