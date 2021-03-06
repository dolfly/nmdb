
#include <stdlib.h>		/* for malloc() */
#include <pthread.h>		/* for mutexes */

#include "queue.h"


struct queue *queue_create(void)
{
	struct queue *q;
	pthread_mutexattr_t attr;

	q = malloc(sizeof(struct queue));
	if (q == NULL)
		return NULL;

	q->size = 0;
	q->top = NULL;
	q->bottom = NULL;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
	pthread_mutex_init(&(q->lock), &attr);
	pthread_mutexattr_destroy(&attr);

	pthread_cond_init(&(q->cond), NULL);

	return q;
}

void queue_free(struct queue *q)
{
	struct queue_entry *e;

	/* We know when we're called there is no other possible queue user;
	 * however, we don't have any sane way to tell sparse about this, so
	 * fake the acquisition of the lock to comply with the operations
	 * performed inside. Obviously, it would be completely safe to do the
	 * queue_lock()/unlock(), but it'd be misleading to the reader */
	__acquire(q->lock);
	e = queue_get(q);
	while (e != NULL) {
		queue_entry_free(e);
		e = queue_get(q);
	}
	__release(q->lock);

	pthread_mutex_destroy(&(q->lock));

	free(q);
	return;
}


void queue_lock(struct queue *q)
{
	pthread_mutex_lock(&(q->lock));
}

void queue_unlock(struct queue *q)
{
	pthread_mutex_unlock(&(q->lock));
}

void queue_signal(struct queue *q)
{
	pthread_cond_signal(&(q->cond));
}

int queue_timedwait(struct queue *q, struct timespec *ts)
{
	return pthread_cond_timedwait(&(q->cond), &(q->lock), ts);
}


struct queue_entry *queue_entry_create(void)
{
	struct queue_entry *e;

	e = malloc(sizeof(struct queue_entry));
	if (e == NULL)
		return NULL;

	e->operation = 0;
	e->key = NULL;
	e->val = NULL;
	e->newval = NULL;
	e->ksize = 0;
	e->vsize = 0;
	e->nvsize = 0;
	e->prev = NULL;

	return e;
}

void queue_entry_free(struct queue_entry *e) {
	if (e->req) {
		free(e->req->clisa);
		free(e->req);
	}
	if (e->key)
		free(e->key);
	if (e->val)
		free(e->val);
	if (e->newval)
		free(e->newval);
	free(e);
	return;
}


void queue_put(struct queue *q, struct queue_entry *e)
{
	if (q->top == NULL) {
		q->top = q->bottom = e;
	} else {
		q->top->prev = e;
		q->top = e;
	}
	q->size += 1;
	return;
}

struct queue_entry *queue_get(struct queue *q)
{
	struct queue_entry *e, *t;

	if (q->bottom == NULL)
		return NULL;

	e = q->bottom;
	t = q->bottom->prev;
	q->bottom = t;
	if (t == NULL) {
		/* it's empty now */
		q->top = NULL;
	}
	q->size -= 1;
	return e;
}

int queue_isempty(struct queue *q)
{
	return (q->size == 0);
}

