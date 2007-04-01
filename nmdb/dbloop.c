
#include <pthread.h>		/* threading functions */
#include <time.h>		/* nanosleep() */
#include <errno.h>		/* ETIMEDOUT */
#include <string.h>		/* memcmp() */
#include <stdlib.h>		/* malloc()/free() */
#include <stdio.h>		/* snprintf() */

#include "common.h"
#include "dbloop.h"
#include "be.h"
#include "queue.h"
#include "net-const.h"
#include "req.h"
#include "log.h"
#include "netutils.h"
#include "sparse.h"


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
			errlog("Error in queue_timedwait()");
			/* When the timedwait fails the lock is released, so
			 * we need to properly annotate this case. */
			__release(op_queue->lock);
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
	if (e->operation == REQ_SET) {
		rv = db_set(db, e->key, e->ksize, e->val, e->vsize);
		if (!(e->req->flags & FLAGS_SYNC))
			return;

		if (!rv) {
			e->req->reply_err(e->req, ERR_DB);
			return;
		}
		e->req->reply_mini(e->req, REP_OK);

	} else if (e->operation == REQ_GET) {
		unsigned char *val;
		size_t vsize = 64 * 1024;

		val = malloc(vsize);
		if (val == NULL) {
			e->req->reply_err(e->req, ERR_MEM);
			return;
		}
		rv = db_get(db, e->key, e->ksize, val, &vsize);
		if (rv == 0) {
			e->req->reply_mini(e->req, REP_NOTIN);
			free(val);
			return;
		}
		e->req->reply_long(e->req, REP_OK, val, vsize);
		free(val);

	} else if (e->operation == REQ_DEL) {
		rv = db_del(db, e->key, e->ksize);
		if (!(e->req->flags & FLAGS_SYNC))
			return;

		if (rv == 0) {
			e->req->reply_mini(e->req, REP_NOTIN);
			return;
		}
		e->req->reply_mini(e->req, REP_OK);

	} else if (e->operation == REQ_CAS) {
		unsigned char *dbval;
		size_t dbvsize = 64 * 1024;

		/* Compare */
		dbval = malloc(dbvsize);
		if (dbval == NULL) {
			e->req->reply_err(e->req, ERR_MEM);
			return;
		}
		rv = db_get(db, e->key, e->ksize, dbval, &dbvsize);
		if (rv == 0) {
			e->req->reply_mini(e->req, REP_NOTIN);
			free(dbval);
			return;
		}

		if (e->vsize == dbvsize &&
				memcmp(e->val, dbval, dbvsize) == 0) {
			/* Swap */
			rv = db_set(db, e->key, e->ksize, e->newval, e->nvsize);
			if (!rv) {
				e->req->reply_err(e->req, ERR_DB);
				return;
			}

			e->req->reply_mini(e->req, REP_OK);
			free(dbval);
			return;
		}

		e->req->reply_mini(e->req, REP_NOMATCH);
		free(dbval);

	} else if (e->operation == REQ_INCR) {
		unsigned char *dbval;
		size_t dbvsize = 64 * 1024;
		int64_t intval;

		dbval = malloc(dbvsize);
		if (dbval == NULL) {
			e->req->reply_err(e->req, ERR_MEM);
			return;
		}
		rv = db_get(db, e->key, e->ksize, dbval, &dbvsize);
		if (rv == 0) {
			e->req->reply_mini(e->req, REP_NOTIN);
			free(dbval);
			return;
		}

		/* val must be NULL terminated; see cache_incr() */
		if (dbval && dbval[dbvsize - 1] != '\0') {
			e->req->reply_mini(e->req, REP_NOMATCH);
			free(dbval);
			return;
		}

		intval = strtoll((char *) dbval, NULL, 10);
		intval = intval + * (int64_t *) e->val;

		if (dbvsize < 24) {
			/* We know dbval is long enough because we've
			 * allocated it, so we only change dbvsize */
			dbvsize = 24;
		}

		snprintf((char *) dbval, dbvsize, "%23lld",
				(long long int) intval);

		rv = db_set(db, e->key, e->ksize, dbval, dbvsize);
		if (!rv) {
			e->req->reply_err(e->req, ERR_DB);
			return;
		}

		intval = htonll(intval);
		e->req->reply_long(e->req, REP_OK,
				(unsigned char *) &intval, sizeof(intval));

		free(dbval);

	} else {
		wlog("Unknown op 0x%x\n", e->operation);
	}
}

