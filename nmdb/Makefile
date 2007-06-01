
CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifdef STRICT
ALL_CFLAGS += -ansi -pedantic
endif

# prefix for installing the binaries
PREFIX=/usr/local


OBJS = be-qdbm.o cache.o db.o queue.o net.o parse.o tcp.o tipc.o main.o

default: all

all: nmdb

nmdb: $(OBJS)
	$(CC) $(ALL_CFLAGS) $(OBJS) -levent -lpthread -lrt -lqdbm -o nmdb

.c.o:
	$(CC) $(ALL_CFLAGS) -c $< -o $@

install: all
	install -d $(PREFIX)/bin
	install -m 0755 nmdb $(PREFIX)/bin
	install -d $(PREFIX)/man/man1
	install -m 0644 nmdb.1 $(PREFIX)/man/man1/

clean:
	rm -f $(OBJS) nmdb
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default all clean


