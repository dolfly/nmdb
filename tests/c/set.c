
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
	unsigned long s_elapsed;
	nmdb_t *db;

	if (argc != 4) {
		printf("Usage: set-* TIMES KSIZE VSIZE\n");
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
	}
	s_elapsed = timer_stop();

	printf("%lu\n", s_elapsed);

	free(key);
	free(val);
	nmdb_free(db);

	return 0;
}

