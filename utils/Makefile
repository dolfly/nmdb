
CFLAGS += -std=c99 -pedantic -Wall -O3
ALL_CFLAGS = -D_XOPEN_SOURCE=500 $(CFLAGS)

ifdef DEBUG
ALL_CFLAGS += -g
endif

ifdef PROFILE
ALL_CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
endif


# prefix for installing the binaries
PREFIX=/usr/local


ifneq ($(V), 1)
	NICE_CC = @echo "  CC  $@"; $(CC)
else
	NICE_CC = $(CC)
endif


default: all

all: nmdb-stats

nmdb-stats: nmdb-stats.o
	$(NICE_CC) $(ALL_CFLAGS) $< -L../libnmdb -lnmdb -o $@

.c.o:
	$(NICE_CC) $(ALL_CFLAGS) -I../libnmdb -c $< -o $@

install-bin: nmdb-stats
	install -d $(PREFIX)/bin
	install -m 0755 nmdb-stats $(PREFIX)/bin

install-man:
	install -d $(PREFIX)/man/man1
	install -m 0644 nmdb-stats.1 $(PREFIX)/man/man1/

install: install-bin install-man

clean:
	rm -f nmdb-stats.o nmdb-stats
	rm -f *.bb *.bbg *.da *.gcov *.gcda *.gcno gmon.out

.PHONY: default all clean install-bin install-man install


