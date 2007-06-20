
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
	unsigned char *key;
	size_t ksize;
	unsigned long d_elapsed;
	nmdb_t *db;

	if (argc != 3) {
		printf("Usage: test2 TIMES KSIZE\n");
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

	if (key == NULL) {
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
		r = NDEL(db, key, ksize);
		if (r < 0) {
			perror("Del");
			return 1;
		}
	}
	d_elapsed = timer_stop();
	printf("%lu\n", d_elapsed);

	free(key);
	nmdb_free(db);

	return 0;
}

