
ENABLE_TIPC = 1
ENABLE_TCP = 1
ENABLE_UDP = 1

CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 -fPIC $(CFLAGS)
ALL_CFLAGS += -DENABLE_TIPC=$(ENABLE_TIPC) \
		-DENABLE_TCP=$(ENABLE_TCP) \
		-DENABLE_UDP=$(ENABLE_UDP)

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


OBJS = libnmdb.o tcp.o tipc.o udp.o


default: all

all: libs


libs: libnmdb.so libnmdb.a

libnmdb.so: $(OBJS)
	$(CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) -o libnmdb.so

libnmdb.a: $(OBJS)
	$(AR) cr libnmdb.a $(OBJS)


install-lib: libs
	install -d $(PREFIX)/lib
	install -m 0755 libnmdb.so $(PREFIX)/lib
	install -m 0755 libnmdb.a $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 0644 nmdb.h $(PREFIX)/include
	@echo
	@echo "Please run ldconfig to update your library cache"
	@echo

install-man:
	install -d $(PREFIX)/man/man3
	install -m 0644 libnmdb.3 $(PREFIX)/man/man3/

install: install-lib install-man


.c.o:
	$(CC) $(ALL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libnmdb.so libnmdb.a
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default all libs install-lib install-man install clean


