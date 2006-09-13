
#include <pthread.h>		/* threading functions */
#include <time.h>		/* nanosleep() */
#include <errno.h>		/* ETIMEDOUT */

#include <stdio.h>

#include "common.h"
#include "db.h"
#include "be.h"
#include "queue.h"
#include "net-const.h"


static void *db_loop(void *arg);
static void process_op(db_t *db, struct queue_entry *e);


/* Used to signal the loop that it should exit when the queue becomes empty.
 * It's not the cleanest way, but it's simple and effective. */
static int loop_should_stop = 0;


pthread_t *db_loop_start(db_t *db)
{
	pthread_t *thread;

	thread = malloc(sizeof(pthread_t));
	if (thread == NULL)
		return NULL;

	pthread_create(thread, NULL, db_loop, (void *) db);

	return thread;
}

void db_loop_stop(pthread_t *thread)
{
	loop_should_stop = 1;
	pthread_join(*thread, NULL);
	free(thread);
	return;
}


static void *db_loop(void *arg)
{
	int rv;
	struct timespec ts;
	struct queue_entry *e;
	db_t *db;

	db = (db_t *) arg;

	for (;;) {
		/* Condition waits are specified with absolute timeouts, see
		 * pthread_cond_timedwait()'s SUSv3 specification for more
		 * information. We need to calculate it each time.
		 * We sleep for 1 sec. There's no real need for it to be too
		 * fast (it's only used so that stop detection doesn't take
		 * long), but we don't want it to be too slow either. */
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 1;

		rv = 0;
		queue_lock(op_queue);
		while (queue_isempty(op_queue) && rv == 0) {
			rv = queue_timedwait(op_queue, &ts);
		}

		if (rv != 0 && rv != ETIMEDOUT) {
			perror("Error in queue_timedwait()");
			continue;
		}

		e = queue_get(op_queue);
		queue_unlock(op_queue);

		if (e == NULL) {
			if (loop_should_stop) {
				break;
			} else {
				continue;
			}
		}

		process_op(db, e);

		/* Free the entry that was allocated when tipc queued the
		 * operation. This also frees it's components. */
		queue_entry_free(e);
	}

	return NULL;
}

static void process_op(db_t *db, struct queue_entry *e)
{
	int rv;
	if (e->operation == REQ_SET_SYNC) {
		rv = db_set(db, e->key, e->ksize, e->val, e->vsize);
		if (!rv) {
			tipc_reply_err(e->req, ERR_DB);
			return;
		}
		tipc_reply_set(e->req, REP_OK);

	} else if (e->operation == REQ_SET_ASYNC) {
		db_set(db, e->key, e->ksize, e->val, e->vsize);

	} else if (e->operation == REQ_GET) {
		unsigned char *val;
		size_t vsize = 128 * 1024;

		val = malloc(vsize);
		if (val == NULL) {
			tipc_reply_err(e->req, ERR_MEM);
			return;
		}
		rv = db_get(db, e->key, e->ksize, val, &vsize);
		if (rv == 0) {
			tipc_reply_get(e->req, REP_NOTIN, NULL, 0);
			free(val);
			return;
		}
		tipc_reply_get(e->req, REP_OK, val, vsize);
		free(val);

	} else if (e->operation == REQ_DEL_SYNC) {
		rv = db_del(db, e->key, e->ksize);
		if (rv == 0) {
			tipc_reply_del(e->req, REP_NOTIN);
			return;
		}
		tipc_reply_del(e->req, REP_OK);

	} else if (e->operation == REQ_DEL_ASYNC) {
		db_del(db, e->key, e->ksize);

	} else {
		printf("Unknown op 0x%x\n", e->operation);
	}
}

