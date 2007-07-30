
# Protocols to enable
ENABLE_TIPC = 1
ENABLE_TCP = 1
ENABLE_UDP = 1

# Backend to use, can be qdbm or null
BACKEND = qdbm

CFLAGS += -std=c99 -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 $(CFLAGS)
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


OBJS = cache.o dbloop.o queue.o net.o parse.o main.o
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

ifeq ($(BACKEND), qdbm)
	OBJS += be-qdbm.o
	ALL_CFLAGS += -DBACKEND_QDBM
	LIBS += -lqdbm
else ifeq ($(BACKEND), null)
	OBJS += be-null.o
	ALL_CFLAGS += -DBACKEND_NULL
endif


default: all

all: nmdb

nmdb: $(OBJS)
	$(CC) $(ALL_CFLAGS) $(OBJS) $(LIBS) -o nmdb

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


