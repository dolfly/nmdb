
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
	unsigned long s_elapsed;
	char *key = "k";
	char *initval = "0";
	size_t ksize;
	long long int increment;
	int64_t newval;
	nmdb_t *db;

	if (argc != 3) {
		printf("Usage: incr-* TIMES INCREMENT\n");
		return 1;
	}

	times = atoi(argv[1]);
	if (times < 1) {
		printf("Error: TIMES must be >= 1\n");
		return 1;
	}

	increment = strtoll(argv[2], NULL, 10);

	db = nmdb_init();
	if (db == NULL) {
		perror("nmdb_init() failed");
		return 1;
	}

	NADDSRV(db);

	ksize = strlen(key) + 1;

	/* initial set */
	NSET(db, (unsigned char *) key, ksize,
			(unsigned char *) initval, strlen(initval) + 1);

	timer_start();
	for (i = 0; i < times; i++) {
		r = NINCR(db, (unsigned char *) key, ksize, increment,
				&newval);
		if (r != 2) {
			printf("result: %d %lld\n", r, (long long) newval);
			perror("Incr");
			return 1;
		}
	}
	s_elapsed = timer_stop();

	printf("%lu %lld\n", s_elapsed, (long long) newval);

	nmdb_free(db);

	return 0;
}

