
#include <stdlib.h>		/* for malloc() */
#include <pthread.h>		/* for mutexes */

#include "queue.h"


struct queue *queue_create()
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

	return q;
}


void queue_free(struct queue *q)
{
	struct queue_entry *e;

	e = queue_get(q);
	while (e != NULL) {
		queue_entry_free(e);
		e = queue_get(q);
	}

	pthread_mutex_destroy(&(q->lock));

	free(q);
	return;
}


#define queue_lock(q) do { pthread_mutex_lock(&((q)->lock)); } while (0)
#define queue_unlock(q) do { pthread_mutex_unlock(&((q)->lock)); } while (0)


struct queue_entry *queue_entry_create()
{
	struct queue_entry *e;

	e = malloc(sizeof(struct queue_entry));
	if (e == NULL)
		return NULL;

	e->operation = 0;
	e->key = NULL;
	e->val = NULL;
	e->ksize = 0;
	e->vsize = 0;
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
	free(e);
	return;
}


void queue_put(struct queue *q, struct queue_entry *e)
{
	queue_lock(q);
	if (q->top == NULL) {
		q->top = q->bottom = e;
	} else {
		q->top->prev = e;
		q->top = e;
	}
	q->size += 1;
	queue_unlock(q);
	return;
}


struct queue_entry *queue_get(struct queue *q)
{
	struct queue_entry *e, *t;

	queue_lock(q);
	if (q->bottom == NULL) {
		e = NULL;
	} else {
		e = q->bottom;
		t = q->bottom->prev;
		q->bottom = t;
		if (t == NULL) {
			/* it's empty now */
			q->top = NULL;
		}
	}
	q->size -= 1;
	queue_unlock(q);
	return e;
}


