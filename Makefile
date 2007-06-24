
all: default

default: nmdb libnmdb

nmdb:
	make -C nmdb

libnmdb:
	make -C libnmdb

install:
	make -C nmdb install
	make -C libnmdb install

clean:
	make -C nmdb clean
	make -C libnmdb clean


python:
	cd bindings/python && python setup.py build

python_install:
	cd bindings/python && python setup.py install

python_clean:
	cd bindings/python && rm -rf build/


.PHONY: default all clean nmdb libnmdb python python_install python_clean


