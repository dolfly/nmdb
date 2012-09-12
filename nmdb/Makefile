
# Protocols to enable
ENABLE_TCP = 1
ENABLE_UDP = 1
ENABLE_TIPC := $(shell if echo "\#include <linux/tipc.h>" | \
		$(CPP) - > /dev/null 2>&1; then echo 1; else echo 0; fi)
ENABLE_SCTP := $(shell if echo "\#include <netinet/sctp.h>" | \
		$(CPP) - > /dev/null 2>&1; then echo 1; else echo 0; fi)

# Backends to enable
BE_ENABLE_QDBM := $(shell if `pkg-config --exists qdbm`; \
	then echo 1; else echo 0; fi)
BE_ENABLE_BDB := $(shell if echo "\#include <db.h>" | \
		$(CPP) - > /dev/null 2>&1; then echo 1; else echo 0; fi)
BE_ENABLE_TC := $(shell if `pkg-config --exists tokyocabinet`; \
	then echo 1; else echo 0; fi)
BE_ENABLE_TDB := $(shell if `pkg-config --exists tdb`; \
	then echo 1; else echo 0; fi)
BE_ENABLE_LEVELDB := $(shell if echo "\#include <leveldb/c.h>" | \
		$(CPP) - > /dev/null 2>&1; then echo 1; else echo 0; fi)
BE_ENABLE_NULL := 1


CFLAGS += -std=c99 -pedantic -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=600 $(CFLAGS)
ALL_CFLAGS += -DENABLE_TIPC=$(ENABLE_TIPC) \
		-DENABLE_TCP=$(ENABLE_TCP) \
		-DENABLE_UDP=$(ENABLE_UDP) \
		-DENABLE_SCTP=$(ENABLE_SCTP) \
		-DBE_ENABLE_QDBM=$(BE_ENABLE_QDBM) \
		-DBE_ENABLE_BDB=$(BE_ENABLE_BDB) \
		-DBE_ENABLE_TC=$(BE_ENABLE_TC) \
		-DBE_ENABLE_TDB=$(BE_ENABLE_TDB) \
		-DBE_ENABLE_LEVELDB=$(BE_ENABLE_LEVELDB) \
		-DBE_ENABLE_NULL=$(BE_ENABLE_NULL) \


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


OBJS = cache.o dbloop.o queue.o log.o net.o netutils.o parse.o stats.o main.o \
       be.o be-bdb.o be-null.o be-qdbm.o be-tc.o be-tdb.o be-leveldb.o
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


ifeq ($(BE_ENABLE_QDBM), 1)
	ALL_CFLAGS += `pkg-config qdbm --cflags`
	LIBS += `pkg-config qdbm --libs-only-L` -lqdbm
endif
ifeq ($(BE_ENABLE_BDB), 1)
	LIBS += -ldb
endif
ifeq ($(BE_ENABLE_TC), 1)
	ALL_CFLAGS += `pkg-config tokyocabinet --cflags`
	LIBS += `pkg-config tokyocabinet --libs`
endif
ifeq ($(BE_ENABLE_TDB), 1)
	ALL_CFLAGS += `pkg-config tdb --cflags`
	LIBS += `pkg-config tdb --libs`
endif
ifeq ($(BE_ENABLE_LEVELDB), 1)
	LIBS += -lleveldb
endif
ifeq ($(BE_ENABLE_NULL), 1)
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

