
CFLAGS += -std=c99 -Wall -D_XOPEN_SOURCE=500 -O3 -fPIC

ifdef DEBUG
CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifdef STRICT
CFLAGS += -ansi -pedantic
endif

# prefix for installing the binaries
PREFIX=/usr/local


OBJS = libnmdb.o test1c.o test1d.o test2c.o test2d.o


default: all

all: libs


libs: libnmdb.so libnmdb.a

libnmdb.so: libnmdb.o
	$(CC) $(CFLAGS) libnmdb.o -shared -fPIC -o libnmdb.so

libnmdb.a: libnmdb.o
	$(AR) cr libnmdb.a libnmdb.o


tests: test1c test1d test2c test2d

test1c: test1c.o libnmdb.a
	$(CC) $(CFLAGS) test1c.o libnmdb.a -o test1c

test1d: test1d.o libnmdb.a
	$(CC) $(CFLAGS) test1d.o libnmdb.a -o test1d

test2c: test2c.o libnmdb.a
	$(CC) $(CFLAGS) test2c.o libnmdb.a -o test2c

test2d: test2d.o libnmdb.a
	$(CC) $(CFLAGS) test2d.o libnmdb.a -o test2d


install: libs
	install -d $(PREFIX)/lib
	install -m 0755 libnmdb.so $(PREFIX)/lib
	install -m 0755 libnmdb.a $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 0644 nmdb.h $(PREFIX)/include
	install -d $(PREFIX)/man/man3
	install -m 0644 libnmdb.3 $(PREFIX)/man/man3/
	@echo
	@echo "Please run ldconfig to update your library cache"
	@echo


.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libnmdb.so libnmdb.a test1c test1d test2c test2d
	rm -f *.bb *.bbg *.da *.gcov gmon.out

.PHONY: default all libs tests install clean


