#!/usr/bin/env python

import sys
import nmdb
from random import randint, choice


class Mismatch (Exception):
	pass


# network db
ndb = nmdb.Cache()
ndb.add_tipc_server()
ndb.add_tcp_server('localhost')
ndb.add_udp_server('localhost')

# local db
ldb = {}

# history of each key
history = {}

# check decorator
def checked(f):
	def newf(k, *args, **kwargs):
		try:
			return f(k, *args, **kwargs)
		except:
			if k in history:
				print(history[k])
			else:
				print('No history for key', k)
			raise
	newf.__name__ = f.__name__
	return newf


# operations
@checked
def set(k, v):
	ndb[k] = v
	ldb[k] = v
	if k not in history:
		history[k] = []
	history[k].append((set, k, v))

@checked
def get(k):
	try:
		n = ndb[k]
	except KeyError:
		del ldb[k]
		del history[k]
		return 0

	l = ldb[k]
	if l != n:
		raise Mismatch((n, l))
	history[k].append((get, k))
	return True

@checked
def delete(k):
	del ldb[k]
	try:
		del ndb[k]
	except KeyError:
		pass
	history[k].append((delete, k))

def find_missing():
	misses = 0
	for k in list(ldb.keys()):
		if not get(k):
			misses += 1
	return misses

# Use integers because the normal random() generates floating point numbers,
# and they can mess up comparisons because of architecture details.
def getrand():
	return randint(0, 1000000000000000000)


if __name__ == '__main__':
	if len(sys.argv) < 2:
		print('Use: random1-cache.py number_of_keys [key_prefix]')
		sys.exit(1)

	nkeys = int(sys.argv[1])
	if len(sys.argv) > 2:
		key_prefix = sys.argv[2]
	else:
		key_prefix = ''

	# fill all the keys
	print('populate')
	for i in range(nkeys):
		set(key_prefix + str(getrand()), getrand())

	print('missing', find_missing())

	lkeys = list(ldb.keys())

	# operate on them a bit
	print('random operations')
	operations = ('set', 'get', 'delete')
	for i in range(nkeys // 2):
		op = choice(operations)
		k = choice(lkeys)
		if op == 'set':
			set(k, getrand())
		elif op == 'get':
			get(k)
		elif op == 'delete':
			delete(k)
			lkeys.remove(k)

	print('missing', find_missing())

	print('delete')
	for k in lkeys:
		delete(k)

	print('missing', find_missing())

	sys.exit(0)


