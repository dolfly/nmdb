
#ifndef _QUEUE_H
#define _QUEUE_H

#include <pthread.h>		/* for mutexes */
#include <stdint.h>		/* for uint32_t */
#include "req.h"		/* for req_info */

struct queue {
	pthread_mutex_t lock;
	pthread_cond_t cond;

	size_t size;
	struct queue_entry *top, *bottom;
};

struct queue_entry {
	uint32_t operation;
	struct req_info *req;

	unsigned char *key;
	unsigned char *val;
	unsigned char *newval;
	size_t ksize;
	size_t vsize;
	size_t nvsize;

	struct queue_entry *prev;
	/* A pointer to the next element on the list is actually not
	 * necessary, because it's not needed for put and get.
	 */
};


struct queue *queue_create();
void queue_free(struct queue *q);

struct queue_entry *queue_entry_create();
void queue_entry_free(struct queue_entry *e);

void queue_lock(struct queue *q);
void queue_unlock(struct queue *q);
void queue_signal(struct queue *q);
int queue_timedwait(struct queue *q, struct timespec *ts);

void queue_put(struct queue *q, struct queue_entry *e);
struct queue_entry *queue_get(struct queue *q);
int queue_isempty(struct queue *q);

#endif

