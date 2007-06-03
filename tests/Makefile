
CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 -fPIC $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifdef STRICT
ALL_CFLAGS += -ansi -pedantic
endif


OBJS = test1c.o test1d.o test2c.o test2cm.o test2d.o test2dm.o


default: all

all: tests

tests: test1c test1d test2c test2cm test2d test2dm

test1c: test1c.o
	$(CC) $(ALL_CFLAGS) test1c.o -lnmdb -o test1c

test1d: test1d.o
	$(CC) $(ALL_CFLAGS) test1d.o -lnmdb -o test1d

test2c: test2c.o
	$(CC) $(ALL_CFLAGS) test2c.o -lnmdb -o test2c

test2cm: test2cm.o
	$(CC) $(ALL_CFLAGS) test2cm.o -lnmdb -o test2cm

test2d: test2d.o
	$(CC) $(ALL_CFLAGS) test2d.o -lnmdb -o test2d

test2dm: test2dm.o
	$(CC) $(ALL_CFLAGS) test2dm.o -lnmdb -o test2dm


.c.o:
	$(CC) $(ALL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -f test1c test1d test2c test2cm test2d test2dm
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default all tests clean


