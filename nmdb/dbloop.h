
#ifndef _DBLOOP_H
#define _DBLOOP_H

#include <pthread.h>		/* for pthread_t */
#include "be.h"			/* for struct db_conn */

pthread_t *db_loop_start(struct db_conn *db);
void db_loop_stop(pthread_t *thread);

#endif

