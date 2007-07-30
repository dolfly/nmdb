
#ifndef _DBLOOP_H
#define _DBLOOP_H

#include <pthread.h>		/* for pthread_t */
#include "be.h"			/* for db_t */

pthread_t *db_loop_start(db_t *db);
void db_loop_stop(pthread_t *thread);

#endif

