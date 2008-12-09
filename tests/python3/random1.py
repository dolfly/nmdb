#!/usr/bin/env python

import sys
import nmdb
from random import randint, choice


class Mismatch (Exception):
	pass


# network db
ndb = nmdb.DB()
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
	n = ndb[k]
	l = ldb[k]
	if l != n:
		raise Mismatch((n, l))
	history[k].append((get, k))
	return n

@checked
def delete(k):
	del ndb[k]
	del ldb[k]
	history[k].append((delete, k))

@checked
def cas(k, ov, nv):
	prel = ldb[k]
	pren = ndb[k]
	n = ndb.cas(k, ov, nv)
	if k not in ldb:
		l = 0
	elif ldb[k] == ov:
		ldb[k] = nv
		l = 2
	else:
		l = 1
	if n != l:
		print(k, ldb[k], ndb[k])
		print(prel, pren)
		print(history[k])
		raise Mismatch((n, l))
	history[k].append((cas, k, ov, nv))
	return n


def check():
	for k in ldb.keys():
		try:
			n = ndb[k]
			l = ldb[k]
		except:
			print(history[k])
			raise Mismatch((n, l))

		if n != n:
			print(history[k])
			raise Mismatch((n, l))


# Use integers because the normal random() generates floating point numbers,
# and they can mess up comparisons because of architecture details.
def getrand():
	return randint(0, 1000000000000000000)


if __name__ == '__main__':
	if len(sys.argv) < 2:
		print('Use: random1.py number_of_keys [key_prefix]')
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

	lkeys = list(ldb.keys())

	# operate on them a bit
	print('random operations')
	operations = ('set', 'delete', 'cas0', 'cas1')
	for i in range(nkeys // 2):
		op = choice(operations)
		k = choice(lkeys)
		if op == 'set':
			set(k, getrand())
		elif op == 'delete':
			delete(k)
			lkeys.remove(k)
		elif op == 'cas0':
			# unsucessful cas
			cas(k, -1, -1)
		elif op == 'cas1':
			# successful cas
			cas(k, ldb[k], getrand())

	print('check')
	check()

	print('delete')
	for k in lkeys:
		delete(k)

	print('check')
	check()

	sys.exit(0)




