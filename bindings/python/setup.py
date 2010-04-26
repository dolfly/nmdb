
import sys
from distutils.core import setup, Extension

if sys.version_info[0] == 2:
	ver_define = ('PYTHON2', '1')
elif sys.version_info[0] == 3:
	ver_define = ('PYTHON3', '1')


nmdb_ll = Extension("nmdb_ll",
		libraries = ['nmdb'],
		sources = ['nmdb_ll.c'],
		define_macros = [ver_define])

setup(
	name = 'nmdb',
	description = "libnmdb bindings",
	author = "Alberto Bertogli",
	author_email = "albertito@blitiri.com.ar",
	url = "http://blitiri.com.ar/p/nmdb",
	py_modules = ['nmdb'],
	ext_modules = [nmdb_ll]
)

