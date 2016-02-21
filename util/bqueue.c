/*
 * bqueue.c - bounded queue, implemented through bounded circular buffer
 */

#include "bqueue.h"

//#include <assert.h>

void bq_init(BQueue* buffer) {
	bq_clear(buffer);
}

char bq_first(BQueue* bq) {
	return bq->buffer[bq->first];
}

char bq_pop(BQueue* q) {
	//assert(q->size > 0);
	char c = q->buffer[q->first];
	q->first = (q->first + 1) % CAPACITY;
	q->size--;
	return c;
}

char bq_pop_back(BQueue* q) {
	q->size--;
	return q->buffer[(q->first + q->size) % CAPACITY];
}

int bq_push(BQueue* q, char c) {
	//assert(size < capacity);
	if (q->size >= CAPACITY) {
		return -1;
	}

	q->buffer[(q->first + q->size) % CAPACITY] = c;
	q->size++;
	return 0;
}

int bq_empty(BQueue* q) {
	return q->size == 0;
}

void bq_clear(BQueue* buffer) {
	buffer->size = 0;
	buffer->first = 0;
}

void bq_dump(BQueue* q, char* c) {
    int i = q->first;
    int j = 0;
    while (q->size > 0) {
        c[j++] = q->buffer[i++];

        i = i % CAPACITY;
        q->size --;
    }
}
