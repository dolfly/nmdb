
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
	unsigned char *key, *val;
	size_t ksize, vsize;
	unsigned long elapsed, misses = 0;
	nmdb_t *db;

	if (argc != 4) {
		printf("Usage: test2 TIMES KSIZE VSIZE\n");
		return 1;
	}

	times = atoi(argv[1]);
	ksize = atoi(argv[2]);
	vsize = atoi(argv[3]);
	key = malloc(ksize);
	memset(key, 0, ksize);
	val = malloc(vsize);
	memset(val, 0, vsize);

	db = nmdb_init(-1);
	if (db == NULL) {
		perror("nmdb_init() failed");
		return 1;
	}

	printf("set... ");
	timer_start();
	for (i = 0; i < times; i++) {
		* (int *) key = i;
		* (int *) val = i;
		r = nmdb_set(db, key, ksize, val, vsize);
		if (r < 0) {
			perror("Set");
			return 1;
		}
	}
	elapsed = timer_stop();
	printf("%lu\n", elapsed);

	memset(key, 0, ksize);
	free(val);
	val = malloc(128 * 1024);
	printf("get... ");
	timer_start();
	for (i = 0; i < times; i++) {
		* (int *) key = i;
		r = nmdb_get(db, key, ksize, val, vsize);
		if (r < 0) {
			perror("Get");
			return 1;
		} else if (r == 0) {
			misses++;
		}
	}
	elapsed = timer_stop();
	printf("%lu\n", elapsed);
	free(val);

	printf("get misses: %ld\n", misses);

	printf("del... ");
	timer_start();
	for (i = 0; i < times; i++) {
		* (int *) key = i;
		r = nmdb_del(db, key, ksize);
		if (r < 0) {
			perror("Del");
			return 1;
		}
	}
	elapsed = timer_stop();
	printf("%lu\n", elapsed);

	free(key);
	nmdb_free(db);

	return 0;
}

