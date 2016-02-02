#ifndef PQUEUE_H
#define PQUEUE_H

// Capacity size is completely arbitrary
#define PQCAPACITY 50

typedef struct PQueue {
	unsigned int size;		// size of buffer
	unsigned int first; 	// index of first element
	
	void* buffer[PQCAPACITY];

} PQueue;

void pq_init(PQueue* buffer);
void* pq_first(PQueue* buffer);
void* pq_pop(PQueue* buffer);
void* pq_pop_back(PQueue* buffer);
int pq_push(PQueue* buffer, void* ptr);
int pq_empty(PQueue* buffer);
void pq_clear(PQueue* buffer);	// clears buffer (sets first = size = 0)

#endif
