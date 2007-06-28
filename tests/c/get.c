
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
	size_t ksize, vsize;
	unsigned long g_elapsed, misses = 0;
	nmdb_t *db;

	if (argc != 3) {
		printf("Usage: get-* TIMES KSIZE\n");
		return 1;
	}

	times = atoi(argv[1]);
	ksize = atoi(argv[2]);
	if (times < 1) {
		printf("Error: TIMES must be >= 1\n");
		return 1;
	}
	if (ksize < sizeof(int)) {
		printf("Error: KSIZE must be >= sizeof(int)\n");
		return 1;
	}

	key = malloc(ksize);
	memset(key, 0, ksize);
	vsize = 70 * 1024;
	val = malloc(vsize);
	memset(val, 0, vsize);

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
		r = NGET(db, key, ksize, val, vsize);
		if (r <= -2) {
			perror("Get");
			return 1;
		} else if (r == -1) {
			misses++;
		}
	}
	g_elapsed = timer_stop();

	printf("%lu m:%lu\n", g_elapsed, misses);

	free(key);
	free(val);
	nmdb_free(db);

	return 0;
}

