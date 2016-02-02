/*
 * bqueue.c - bounded queue, implemented through bounded circular buffer
 */

#include "bqueue.h"

//#include <assert.h>

void bq_init(struct BQueue* buffer) {
	bq_clear(buffer);
}

char bq_first(struct BQueue* bq) {
	return bq->buffer[bq->first];
}

char bq_pop(struct BQueue* q) {
	//assert(q->size > 0);
	char c = q->buffer[q->first];
	q->first = (q->first + 1) % CAPACITY;
	q->size--;
	return c;
}

char bq_pop_back(struct BQueue* q) {
	q->size--;
	return q->buffer[(q->first + q->size) % CAPACITY];
}

int bq_push(struct BQueue* q, char c) {
	//assert(size < capacity);
	if (q->size >= CAPACITY) {
		return -1;
	}

	q->buffer[(q->first + q->size) % CAPACITY] = c;
	q->size++;
	return 0;
}

int bq_empty(struct BQueue* q) {
	return q->size == 0;
}

void bq_clear(struct BQueue* buffer) {
	buffer->size = 0;
	buffer->first = 0;
}
