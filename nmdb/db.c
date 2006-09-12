
#include <pthread.h>		/* threading functions */
#include <time.h>		/* nanosleep() */

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
	struct queue_entry *e;
	struct timespec ts;
	db_t *db;

	db = (db_t *) arg;

	/* We will sleep this amount of time when the queue is empty. It's
	 * hardcoded, but needs testing. Currenly 0.2s. */
	ts.tv_sec = 0;
	ts.tv_nsec = 1000000 / 5;

	for (;;) {
		e = queue_get(op_queue);
		if (e == NULL) {
			if (loop_should_stop) {
				break;
			} else {
				nanosleep(&ts, NULL);
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

