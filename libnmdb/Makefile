
ENABLE_TCP = 1
ENABLE_UDP = 1
ENABLE_TIPC := $(shell if echo "\#include <linux/tipc.h>" | \
		$(CPP) - > /dev/null 2>&1; then echo 1; else echo 0; fi)
ENABLE_SCTP := $(shell if echo "\#include <netinet/sctp.h>" | \
		$(CPP) - > /dev/null 2>&1; then echo 1; else echo 0; fi)

CFLAGS += -std=c99 -pedantic -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 -fPIC $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif


# prefix for installing the binaries
PREFIX=/usr/local


OBJS = libnmdb.o netutils.o tcp.o tipc.o udp.o sctp.o


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
else
	NICE_CC = $(CC)
endif


default: all

all: libs libnmdb.pc

internal.h: internal.h.in
	@echo "generating internal.h"
	@cat internal.h.in | \
		sed 's/++CONFIG_ENABLE_TIPC++/$(ENABLE_TIPC)/g' | \
		sed 's/++CONFIG_ENABLE_TCP++/$(ENABLE_TCP)/g' | \
		sed 's/++CONFIG_ENABLE_UDP++/$(ENABLE_UDP)/g' | \
		sed 's/++CONFIG_ENABLE_SCTP++/$(ENABLE_SCTP)/g' \
		> internal.h

libnmdb.pc: libnmdb.skel.pc
	@echo "generating libnmdb.pc"
	@cat libnmdb.skel.pc | \
		sed 's@++PREFIX++@$(PREFIX)@g' \
		> libnmdb.pc

libs: libnmdb.so libnmdb.a

libnmdb.so: internal.h $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) -shared -fPIC $(OBJS) -o libnmdb.so

libnmdb.a: internal.h $(OBJS)
	$(AR) cr libnmdb.a $(OBJS)


install-lib: libs libnmdb.pc
	install -d $(PREFIX)/lib
	install -m 0755 libnmdb.so $(PREFIX)/lib
	install -m 0755 libnmdb.a $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 0644 nmdb.h $(PREFIX)/include
	install -d $(PREFIX)/lib/pkgconfig
	install -m 644 libnmdb.pc $(PREFIX)/lib/pkgconfig
	@echo
	@echo "Please run ldconfig to update your library cache"
	@echo

install-man:
	install -d $(PREFIX)/man/man3
	install -m 0644 libnmdb.3 $(PREFIX)/man/man3/

install: install-lib install-man

doxygen:
	$(MAKE) -C doxygen/

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

clean:
	rm -f internal.h libnmdb.pc $(OBJS) libnmdb.so libnmdb.a
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out
	$(MAKE) -C doxygen $@

.PHONY: default all libs install-lib install-man install doxygen clean


