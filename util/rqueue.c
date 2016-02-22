/*
 * rqueue.c - ring queue, implemented through bounded circular buffer
 */
#include <rqueue.h>

#include <string.h>

//#include <assert.h>

void rq_init(RQueue* q, void* buffer, unsigned int max_size, unsigned int unit_size) {
    q->max_size = max_size;
    q->unit_size = unit_size;
    q->buffer = (unsigned int)buffer;

    rq_clear(q);
}

inline void* rq_first(RQueue* q) {
	return (void*)(q->buffer + q->first * q->unit_size);
}

void* rq_pop_front(RQueue* q) {
	//assert(q->size > 0);
    unsigned int addr = q->buffer + q->first * q->unit_size;

	q->first = (q->first + 1) % q->max_size;
	q->size--;

	return (void*)addr;
}

inline void* rq_pop_back(RQueue* q) {
    return (void *)(q->buffer + ((--q->size + q->first) % q->max_size) * q->unit_size);
}

int rq_push_front(RQueue* q, void* p) {
	if (q->size >= q->max_size) {
		return -1;
	}

    q->first = (q->first - 1) % q->max_size;

    unsigned int destination = q->buffer + q->first * q->unit_size;
    memory_copy(p, q->unit_size, (void *)destination, q->unit_size);

	q->size++;
	return 0;
}

int rq_push_back(RQueue* q, void* p) {
	if (q->size >= q->max_size) {
		return -1;
	}

    unsigned int destination = q->buffer + ((q->size + q->first) % q->max_size) * q->unit_size;
    memory_copy(p, q->unit_size, (void *)destination, q->unit_size);

	q->size++;
	return 0;
}

inline int rq_empty(RQueue* q) {
	return q->size == 0;
}

inline void rq_clear(RQueue* q) {
	q->size = 0;
	q->first = 0;
}

inline void* rq_get(RQueue* q, int index) {
    return (void *)(q->buffer + ((q->first + index) % q->max_size) * q->unit_size);
}
