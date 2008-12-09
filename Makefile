
all: default

default: nmdb libnmdb utils

nmdb:
	$(MAKE) -C nmdb

libnmdb:
	$(MAKE) -C libnmdb

utils:
	$(MAKE) -C utils

install:
	$(MAKE) -C nmdb install
	$(MAKE) -C libnmdb install
	$(MAKE) -C utils install

clean:
	$(MAKE) -C nmdb clean
	$(MAKE) -C libnmdb clean
	$(MAKE) -C utils clean


python:
	cd bindings/python && python setup.py build

python_install:
	cd bindings/python && python setup.py install

python_clean:
	cd bindings/python && rm -rf build/

python3:
	cd bindings/python3 && python3 setup.py build

python3_install:
	cd bindings/python3 && python3 setup.py install

python3_clean:
	cd bindings/python3 && rm -rf build/


.PHONY: default all clean nmdb libnmdb utils \
	python python_install python_clean \
	python3 python3_install python3_clean


