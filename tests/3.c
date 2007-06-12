
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

#include <nmdb.h>
#include "timer.h"
#include "prototypes.h"


int main(int argc, char **argv)
{
	int i, r, times;
	unsigned char *key, *val;
	size_t ksize, vsize, bsize;
	unsigned long elapsed, misses = 0;
	nmdb_t *db;

	if (argc != 4) {
		printf("Usage: test3 TIMES KSIZE VSIZE\n");
		return 1;
	}

	times = atoi(argv[1]);
	ksize = atoi(argv[2]);
	vsize = atoi(argv[3]);
	if (times < 1) {
		printf("Error: TIMES must be >= 1\n");
		return 1;
	}
	if (ksize < sizeof(int) || vsize < sizeof(int)) {
		printf("Error: KSIZE and VSIZE must be >= sizeof(int)\n");
		return 1;
	}

	key = malloc(ksize);
	memset(key, 0, ksize);
	val = malloc(70 * 1024);
	bsize = 70 * 1024;
	memset(val, 0, bsize);

	if (key == NULL || val == NULL) {
		perror("Error: malloc()");
		return 1;
	}

	db = nmdb_init();
	if (db == NULL) {
		perror("nmdb_init() failed");
		return 1;
	}

	NADDSRV(db);

	timer_start();
	for (i = 0; i < times; i++) {
		* (int *) key = i;
		* (int *) val = i;
		r = NSET(db, key, ksize, val, vsize);
		if (r < 0) {
			perror("Set");
			return 1;
		}

		* (int *) key = i;
		r = NGET(db, key, ksize, val, bsize);
		if (r < 0) {
			perror("Get");
			return 1;
		} else if (r == 0) {
			misses++;
		}

		* (int *) key = i;
		r = NDEL(db, key, ksize);
		if (r < 0) {
			perror("Del");
			return 1;
		}
	}
	elapsed = timer_stop();
	printf("%lu %lu\n", elapsed, misses);

	free(key);
	nmdb_free(db);

	return 0;
}

