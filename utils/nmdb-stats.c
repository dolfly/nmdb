
/* nmdb-stats.c
 * Queries the stats of a nmdb server.
 * Alberto Bertogli (albertito@gmail.com)
 */

#include <stdio.h>		/* printf() */
#include <stdint.h>		/* uint64_t */
#include <string.h>		/* strcmp() */
#include <stdlib.h>		/* atoi() */

#include "nmdb.h"


/* ntohll() is not standard, so we define it using an UGLY trick because there
 * is no standard way to check for endianness at runtime! (this is the same as
 * the one in nmdb/parse.c, the infraestructure to keep these common is not
 * worth it)*/
static uint64_t ntohll(uint64_t x)
{
	static int endianness = 0;

	/* determine the endianness by checking how htonl() behaves; use -1
	 * for little endian and 1 for big endian */
	if (endianness == 0) {
		if (htonl(1) == 1)
			endianness = 1;
		else
			endianness = -1;
	}

	if (endianness == 1) {
		/* big endian */
		return x;
	}

	/* little endian */
	return ( ntohl( (x >> 32) & 0xFFFFFFFF ) | \
			( (uint64_t) ntohl(x & 0xFFFFFFFF) ) << 32 );
}

#define MAX_STATS_SIZE 64

void help(void)
{
	printf("Use: nmdb-stats [ tipc port | [tcp|udp|sctp] host port ]\n");
}

int main(int argc, char **argv)
{
	int i, j, k;
	int rv;
	uint64_t stats[MAX_STATS_SIZE];
	unsigned int nservers = 0, nstats = 0;
	nmdb_t *db;

	db = nmdb_init();

	if (argc < 3) {
		help();
		return 1;
	}

	if (strcmp("tipc", argv[1]) == 0) {
		rv = nmdb_add_tipc_server(db, atoi(argv[2]));
	} else {
		if (argc != 4) {
			help();
			return 1;
		}

		if (strcmp("tcp", argv[1]) == 0) {
			rv = nmdb_add_tcp_server(db, argv[2], atoi(argv[3]));
		} else if (strcmp("udp", argv[1]) == 0) {
			rv = nmdb_add_udp_server(db, argv[2], atoi(argv[3]));
		} else if  (strcmp("sctp", argv[1]) == 0) {
			rv = nmdb_add_sctp_server(db, argv[2], atoi(argv[3]));
		} else {
			help();
			return 1;
		}
	}

	if (!rv) {
		perror("Error adding server");
		return 1;
	}

	rv = nmdb_stats(db, (unsigned char *) stats, sizeof(stats),
			&nservers, &nstats);
	if (rv <= 0) {
		printf("Error %d\n", rv);
		return 1;
	}

	/* Macro to simplify showing the fields */
	#define shst(s, pos) \
		do { \
			printf("\t%ju\t%s\n", ntohll(stats[j + pos]), s); \
		} while(0)

	/* The following assumes it can be more than one server. This can
	 * never happen with the current code, but it can be useful as an
	 * example in the future. */
	j = 0;
	for (i = 0; i < nservers; i++) {
		printf("stats for server %d:\n", i);

		j = nstats * i;

		shst("cache get", 0);
		shst("cache set", 1);
		shst("cache del", 2);
		shst("cache cas", 3);
		shst("cache incr", 4);

		shst("db get", 5);
		shst("db set", 6);
		shst("db del", 7);
		shst("db cas", 8);
		shst("db incr", 9);

		shst("cache hits", 10);
		shst("cache misses", 11);

		shst("db hits", 12);
		shst("db misses", 13);

		shst("msg tipc", 14);
		shst("msg tcp", 15);
		shst("msg udp", 16);
		shst("msg sctp", 17);

		shst("version mismatch", 18);
		shst("broken requests", 19);
		shst("unknown requests", 20);

		/* if there are any fields we don't know, show them anyway */
		for (k = 21; k < nstats; k++) {
			shst("unknown field", k);
		}

		printf("\n");
	}

	nmdb_free(db);

	return 0;
}

