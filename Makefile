
all: default

default: nmdb libnmdb utils

nmdb:
	make -C nmdb

libnmdb:
	make -C libnmdb

utils:
	make -C utils

install:
	make -C nmdb install
	make -C libnmdb install
	make -C utils install

clean:
	make -C nmdb clean
	make -C libnmdb clean
	make -C utils clean


python:
	cd bindings/python && python setup.py build

python_install:
	cd bindings/python && python setup.py install

python_clean:
	cd bindings/python && rm -rf build/


.PHONY: default all clean nmdb libnmdb utils python python_install python_clean


