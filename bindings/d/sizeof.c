
/* sizeof.c
 * Used to find out the size of some intresting structures and data types, to
 * help defining D's bindings.
 */

#include <stdio.h>
#include <nmdb.h>

int main()
{
	printf("sizeof(struct nmdb_t) = %lu\n", sizeof(struct nmdb_t));
	return 0;
}

