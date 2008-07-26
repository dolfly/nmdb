
# Protocols to enable
ENABLE_TIPC = 1
ENABLE_TCP = 1
ENABLE_UDP = 1
ENABLE_SCTP = 1

# Backend to use, can be qdbm, bdb, or null
BACKEND = qdbm

CFLAGS += -std=c99 -pedantic -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=600 $(CFLAGS)
ALL_CFLAGS += -DENABLE_TIPC=$(ENABLE_TIPC) \
		-DENABLE_TCP=$(ENABLE_TCP) \
		-DENABLE_UDP=$(ENABLE_UDP) \
		-DENABLE_SCTP=$(ENABLE_SCTP)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -ftest-coverage -fprofile-generate
endif

ifdef PROFILE_USE
ALL_CFLAGS += -fprofile-use
endif


# prefix for installing the binaries
PREFIX=/usr/local


OBJS = cache.o dbloop.o queue.o log.o net.o netutils.o parse.o stats.o main.o
LIBS = -levent -lpthread -lrt


ifeq ($(ENABLE_TIPC), 1)
	OBJS += tipc.o
else
	OBJS += tipc-stub.o
endif

ifeq ($(ENABLE_TCP), 1)
	OBJS += tcp.o
else
	OBJS += tcp-stub.o
endif

ifeq ($(ENABLE_UDP), 1)
	OBJS += udp.o
else
	OBJS += udp-stub.o
endif

ifeq ($(ENABLE_SCTP), 1)
	OBJS += sctp.o
else
	OBJS += sctp-stub.o
endif

# Use series of ifeq-endif instead of else-ifeq because otherwise the nesting
# is a mess. Using "else ifeq ..." in the same line is only supported from
# gmake 3.81, which is too new.
ifeq ($(BACKEND), qdbm)
	OBJS += be-qdbm.o
	ALL_CFLAGS += `pkg-config qdbm --cflags` -DBACKEND_QDBM
	LIBS += `pkg-config qdbm --libs-only-L` -lqdbm
endif
ifeq ($(BACKEND), bdb)
	OBJS += be-bdb.o
	ALL_CFLAGS += -DBACKEND_BDB
	LIBS += -ldb
endif
ifeq ($(BACKEND), null)
	OBJS += be-null.o
	ALL_CFLAGS += -DBACKEND_NULL
endif


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
else
	NICE_CC = $(CC)
endif


default: all

all: nmdb

nmdb: $(OBJS)
	$(NICE_CC) $(ALL_CFLAGS) $(OBJS) $(LIBS) -o nmdb

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -c $< -o $@

install-bin: nmdb
	install -d $(PREFIX)/bin
	install -m 0755 nmdb $(PREFIX)/bin

install-man:
	install -d $(PREFIX)/man/man1
	install -m 0644 nmdb.1 $(PREFIX)/man/man1/

install: install-bin install-man

clean: clean-build clean-prof

clean-build:
	rm -f $(OBJS) nmdb

clean-prof:
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default all \
	install-bin install-man install \
	clean-build clean-prof clean

