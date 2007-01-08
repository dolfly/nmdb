
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
	unsigned long s_elapsed, g_elapsed, d_elapsed, misses = 0;
	nmdb_t *db;

	if (argc != 4) {
		printf("Usage: test2 TIMES KSIZE VSIZE\n");
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
	val = malloc(vsize);
	memset(val, 0, vsize);

	if (key == NULL || val == NULL) {
		perror("Error: malloc()");
		return 1;
	}

	db = nmdb_init(-1);
	if (db == NULL) {
		perror("nmdb_init() failed");
		return 1;
	}

	if (!nmdb_add_server(db, 11) ||
			!nmdb_add_server(db, 12) ||
			!nmdb_add_server(db, 13)) {
		perror("nmdb_add_server() failed");
		return 1;
	}

	timer_start();
	for (i = 0; i < times; i++) {
		* (int *) key = i;
		* (int *) val = i;
		r = nmdb_cache_set(db, key, ksize, val, vsize);
		if (r < 0) {
			perror("Set");
			return 1;
		}
	}
	s_elapsed = timer_stop();

	memset(key, 0, ksize);
	gval = malloc(70 * 1024);
	timer_start();
	for (i = 0; i < times; i++) {
		* (int *) key = i;
		r = nmdb_cache_get(db, key, ksize, gval, vsize);
		if (r < 0) {
			perror("Get");
			return 1;
		} else if (r == 0) {
			misses++;
			continue;
		}
		* (int *) val = i;
		if (memcmp((void *) val, (void *) gval, vsize) != 0) {
			printf("Values differ for key %s: %s - %s\n",
					key, val, gval);
			printf("i: %d\n", i);
			return 1;
		}
	}
	g_elapsed = timer_stop();
	free(gval);
	free(val);

	timer_start();
	for (i = 0; i < times; i++) {
		* (int *) key = i;
		r = nmdb_cache_del(db, key, ksize);
		if (r < 0) {
			perror("Del");
			return 1;
		}
	}
	d_elapsed = timer_stop();
	printf("%lu %lu %lu %lu\n", s_elapsed, g_elapsed, d_elapsed, misses);

	free(key);
	nmdb_free(db);

	return 0;
}

