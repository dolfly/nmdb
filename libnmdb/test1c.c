
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

#include "nmdb.h"
#include "timer.h"


int main(int argc, char **argv)
{
	int i, r, times;
	unsigned char *key, *val, *gval;
	size_t ksize, vsize;
	unsigned long elapsed, misses = 0;
	nmdb_t *db;

	if (argc != 4) {
		printf("Usage: test1 TIMES KEY VAL\n");
		return 1;
	}

	times = atoi(argv[1]);
	key = (unsigned char *) argv[2];
	ksize = strlen((char *) key);
	val = (unsigned char *) argv[3];
	vsize = strlen((char *) val);

	db = nmdb_init(-1);
	if (db == NULL) {
		perror("nmdb_init() failed");
		return 1;
	}

	printf("set... ");
	timer_start();
	for (i = 0; i < times; i++) {
		r = nmdb_cache_set(db, key, ksize, val, vsize);
		if (r < 0) {
			perror("Set");
			return 1;
		}
	}
	elapsed = timer_stop();
	printf("%lu\n", elapsed);

	gval = malloc(128 * 1024);
	printf("get... ");
	timer_start();
	for (i = 0; i < times; i++) {
		r = nmdb_cache_get(db, key, ksize, gval, vsize);
		if (r < 0) {
			perror("Get");
			return 1;
		} else if (r == 0) {
			misses++;
		}
	}
	elapsed = timer_stop();
	printf("%lu\n", elapsed);
	free(gval);

	printf("get misses: %ld\n", misses);

	printf("del... ");
	timer_start();
	for (i = 0; i < times; i++) {
		r = nmdb_cache_del(db, key, ksize);
		if (r < 0) {
			perror("Del");
			return 1;
		}
	}
	elapsed = timer_stop();
	printf("%lu\n", elapsed);


	nmdb_free(db);

	return 0;
}

