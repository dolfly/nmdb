#!/usr/bin/env python

"""
This is a stress test for the nmdb server, library, and Python (2 and 3)
bindings.

It creates a number of keys, and then operates randomly on them, verifying
that everything is working as expected.

It can operate with the database or only with the cache.

You can run it with both Python 2 and 3.
"""

import sys
import nmdb
from random import randint, choice


class Mismatch (Exception):
	"Mismatch between local and remote database"
	pass



# History of each key, global because we use it from several places for
# debugging purposes
history = {}

def checked(f):
	"Prints the history of the key when an operation fails"
	def newf(self, k, *args, **kwargs):
		try:
			return f(self, k, *args, **kwargs)
		except:
			if k in history:
				print(history[k])
			else:
				print('No history for key', k)
			raise
	newf.__name__ = f.__name__
	return newf


class Tester:
	"Common code for tester classes"
	def __init__(self):
		# nmdb connection
		self.ndb = self.connect()
		self.ndb.add_tipc_server()
		self.ndb.add_tcp_server('localhost')
		self.ndb.add_udp_server('localhost')

		# local database
		self.ldb = {}

	def connect(self):
		"Connects to an nmdb instance"
		raise NotImplementedError

	@checked
	def set(self, k, v):
		self.ndb[k] = v
		self.ldb[k] = v
		if k not in history:
			history[k] = []
		history[k].append((self.set, k, v))

	def check(self):
		"Checks all entries in ldb match the ones in ndb"
		n = l = None
		for k in self.ldb.keys():
			# get() verifies they match so we do not need to care
			# about that
			self.get(k)

	def get(self, k):
		raise NotImplementedError

	def delete(self, k):
		raise NotImplementedError

	def cas(self, k, ov, nv):
		raise NotImplementedError

	def find_missing(self):
		raise NotImplementedError


class DBTester (Tester):
	"Tester for db mode"

	def connect(self):
		return nmdb.DB()

	@checked
	def get(self, k):
		n = self.ndb[k]
		l = self.ldb[k]
		if l != n:
			raise Mismatch((n, l))
		history[k].append((self.get, k))
		return n

	@checked
	def delete(self, k):
		del self.ndb[k]
		del self.ldb[k]
		history[k].append((self.delete, k))

	@checked
	def cas(self, k, ov, nv):
		prel = self.ldb[k]
		pren = self.ndb[k]
		n = self.ndb.cas(k, ov, nv)
		if k not in self.ldb:
			l = 0
		elif self.ldb[k] == ov:
			self.ldb[k] = nv
			l = 2
		else:
			l = 1
		if n != l:
			print(k, self.ldb[k], self.ndb[k])
			print(prel, pren)
			print(history[k])
			raise Mismatch((n, l))
		history[k].append((self.cas, k, ov, nv))
		return n

	def find_missing(self):
		# we just check we can get them all
		for k in self.ldb.keys():
			self.get(k)
		return 0


class CacheTester (Tester):
	"Tester for cache mode"

	def connect(self):
		return nmdb.Cache()

	@checked
	def get(self, k):
		try:
			n = self.ndb[k]
		except KeyError:
			del self.ldb[k]
			del history[k]
			return False

		l = self.ldb[k]
		if l != n:
			raise Mismatch((n, l))
		history[k].append((self.get, k))
		return True

	@checked
	def delete(self, k):
		del self.ldb[k]
		try:
			del self.ndb[k]
		except KeyError:
			pass
		history[k].append((self.delete, k))

	def find_missing(self):
		misses = 0
		for k in list(self.ldb.keys()):
			if not self.get(k):
				misses += 1
		return misses


# Use integers because the normal random() generates floating point numbers,
# and they can mess up comparisons because of architecture details.
def getrand():
	return randint(0, 1000000000000000000)


def main():
	# We want to always use a generator range(), which has different names
	# in Python 2 and 3, so isolate the code from this hack
	if sys.version_info[0] == 2:
		gen_range = xrange
	else:
		gen_range = range

	if len(sys.argv) < 3:
		print('Use: random1.py db|cache number_of_keys [key_prefix]')
		sys.exit(1)

	mode = sys.argv[1]
	if mode not in ('db', 'cache'):
		print('Error: mode must be either db or cache')
		sys.exit(1)

	nkeys = int(sys.argv[2])
	if len(sys.argv) >= 4:
		key_prefix = sys.argv[3]
	else:
		key_prefix = ''

	if mode == 'db':
		tester = DBTester()
	else:
		tester = CacheTester()

	# fill all the keys
	print('populate')
	for i in gen_range(nkeys):
		tester.set(key_prefix + str(getrand()), getrand())

	print('missing', tester.find_missing())

	lkeys = list(tester.ldb.keys())

	# operate on them a bit
	print('random operations')

	if mode == 'db':
		operations = ('set', 'delete', 'cas0', 'cas1')
	else:
		operations = ('set', 'get', 'delete')

	for i in gen_range(min(len(lkeys), nkeys // 2)):
		op = choice(operations)
		k = choice(lkeys)
		if op == 'set':
			tester.set(k, getrand())
		elif op == 'get':
			tester.get(k)
		elif op == 'delete':
			tester.delete(k)
			lkeys.remove(k)
		elif op == 'cas0':
			# unsucessful cas
			tester.cas(k, -1, -1)
		elif op == 'cas1':
			# successful cas
			tester.cas(k, tester.ldb[k], getrand())

	print('missing', tester.find_missing())

	print('check')
	tester.check()

	print('delete')
	for k in lkeys:
		tester.delete(k)

	print('check')
	tester.check()


if __name__ == '__main__':
	main()

