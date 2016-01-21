/*
 * pqueue.c - pointer queue, implemented through bounded circular buffer
 */

#include "pqueue.h"

//#include <assert.h>

void pq_init(PQueue* buffer) {
	pq_clear(buffer);
}

void* pq_first(PQueue* pq) {
	return pq->buffer[pq->first];
}

void* pq_pop(PQueue* q) {
	//assert(q->size > 0);
	void* p = q->buffer[q->first];
	q->first = (q->first + 1) % PQCAPACITY;
	q->size--;
	return p;
}

void* pq_pop_back(PQueue* q) {
	q->size--;
	return q->buffer[q->first + q->size];
}

int pq_push(PQueue* q, void* p) {
	//assert(size < capacity);
	if (q->size >= PQCAPACITY) {
		return -1;
	}

	q->buffer[(q->first + q->size) % PQCAPACITY] = p;
	q->size++;
	return 0;
}

int pq_empty(PQueue* q) {
	return q->size == 0;
}

void pq_clear(PQueue* buffer) {
	buffer->size = 0;
	buffer->first = 0;
}
