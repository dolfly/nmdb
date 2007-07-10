
BFLAGS += -Wall -O6
ALL_BFLAGS = $(BFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -pg
endif


default: all

all: lib test1

lib:
	bigloo $(ALL_BFLAGS) -c nmdb.scm -o nmdb.o

test1: lib
	bigloo $(ALL_BFLAGS) nmdb.o test1.scm -lnmdb -o test1

clean:
	rm -f nmdb.o test1.o test1

.PHONY: default all clean


