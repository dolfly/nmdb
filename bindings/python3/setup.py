
from distutils.core import setup, Extension

nmdb_ll = Extension("nmdb_ll",
		libraries = ['nmdb'],
		sources = ['nmdb_ll.c'])

setup(
	name = 'nmdb',
	description = "libnmdb bindings",
	author = "Alberto Bertogli",
	author_email = "albertito@blitiri.com.ar",
	url = "http://blitiri.com.ar/p/nmdb",
	py_modules = ['nmdb'],
	ext_modules = [nmdb_ll]
)

