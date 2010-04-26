#!/usr/bin/env python

"""
This tests adds some random keys and then checks if they all appear when
walking through all the keys in the server.

You can run it with both Python 2 and 3.
"""

import sys
import nmdb
from random import randint, choice


class Tester:
	def __init__(self):
		# nmdb connection
		self.ndb = nmdb.DB()
		self.ndb.add_tipc_server()

		# disable autopickle so we don't crash if there are keys in
		# the database that were not stored using it
		self.ndb.autopickle = False

		# local database
		self.ldb = {}

	def set(self, k, v):
		self.ndb[k] = v
		self.ldb[k] = v

	def check(self):
		keys_not_found = set(self.ldb)
		repeated = {}
		unknown = 0

		try:
			k = self.ndb.firstkey()
		except KeyError:
			return None

		while k:
			if k in self.ldb:
				if k in keys_not_found:
					keys_not_found.remove(k)
				else:
					repeated.setdefault(k, 0)
					repeated[k] += 1
			else:
				unknown += 1

			try:
				k = self.ndb.nextkey(k)
			except KeyError:
				break

		return keys_not_found, repeated, unknown

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

	if len(sys.argv) < 2:
		print('Use: random1.py number_of_keys [key_prefix]')
		sys.exit(1)

	nkeys = int(sys.argv[1])
	if len(sys.argv) >= 3:
		key_prefix = sys.argv[2]
	else:
		key_prefix = ''

	tester = Tester()

	# fill all the keys
	print('populate')
	for i in gen_range(nkeys):
		k = bytes( (key_prefix + str(getrand())).encode("utf8") )
		tester.set(k, bytes(str(getrand()).encode("ascii")) )

	print('check')
	keys_not_found, repeated, unknown = tester.check()
	print('  keys not found: %d' % len(keys_not_found))
	print('  repeated: %d' % len(repeated))
	print('  unknown: %d' % unknown)

	print('delete')
	for k in tester.ldb:
		del tester.ndb[k]

if __name__ == '__main__':
	main()

