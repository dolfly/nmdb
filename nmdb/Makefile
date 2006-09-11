
CFLAGS += -std=c99 -Wall -D_XOPEN_SOURCE=500 -O3

ifdef DEBUG
CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif

ifdef STRICT
CFLAGS += -ansi -pedantic
endif

# prefix for installing the binaries
PREFIX=/usr/local


OBJS = be-qdbm.o cache.o db.o queue.o net.o tipc.o main.o

default: all

all: nmdb

nmdb: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -levent -lpthread -lqdbm -o nmdb

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -d $(PREFIX)/bin
	install -m 0755 nmdb $(PREFIX)/bin
	install -d $(PREFIX)/man/man1
	install -m 0644 nmdb.1 $(PREFIX)/man/man1/

clean:
	rm -f $(OBJS) nmdb
	rm -f *.bb *.bbg *.da *.gcov gmon.out

.PHONY: default all clean


