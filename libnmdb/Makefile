
ENABLE_TIPC = 1
ENABLE_TCP = 1

CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 -fPIC $(CFLAGS)
ALL_CFLAGS += -DENABLE_TIPC=$(ENABLE_TIPC) -DENABLE_TCP=$(ENABLE_TCP)

ifdef DEBUG
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifdef STRICT
ALL_CFLAGS += -ansi -pedantic
endif

# prefix for installing the binaries
PREFIX=/usr/local


OBJS = libnmdb.o test1c.o test1d.o test2c.o test2cm.o test2d.o test2dm.o


default: all

all: libs


libs: libnmdb.so libnmdb.a

libnmdb.so: libnmdb.o
	$(CC) $(ALL_CFLAGS) -shared -fPIC libnmdb.o -o libnmdb.so

libnmdb.a: libnmdb.o
	$(AR) cr libnmdb.a libnmdb.o


tests: test1c test1d test2c test2cm test2d test2dm

test1c: test1c.o libnmdb.a
	$(CC) $(ALL_CFLAGS) test1c.o libnmdb.a -o test1c

test1d: test1d.o libnmdb.a
	$(CC) $(ALL_CFLAGS) test1d.o libnmdb.a -o test1d

test2c: test2c.o libnmdb.a
	$(CC) $(ALL_CFLAGS) test2c.o libnmdb.a -o test2c

test2cm: test2cm.o libnmdb.a
	$(CC) $(ALL_CFLAGS) test2cm.o libnmdb.a -o test2cm

test2d: test2d.o libnmdb.a
	$(CC) $(ALL_CFLAGS) test2d.o libnmdb.a -o test2d

test2dm: test2dm.o libnmdb.a
	$(CC) $(ALL_CFLAGS) test2dm.o libnmdb.a -o test2dm


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
	$(CC) $(ALL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libnmdb.so libnmdb.a
	rm -f test1c test1d test2c test2cm test2d test2dm
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default all libs tests install clean


