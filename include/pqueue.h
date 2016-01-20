#ifndef PQUEUE_H
#define PQUEUE_H

// Capacity size is completely arbitrary
#define PQCAPACITY 500

struct PQueue {
	unsigned int size;		// size of buffer
	unsigned int first; 	// index of first element
	
	void* buffer[PQCAPACITY];

};

void pq_init(struct PQueue* buffer);
void* pq_first(struct PQueue* buffer);
void* pq_pop(struct PQueue* buffer);
void* pq_pop_back(struct PQueue* buffer);
int pq_push(struct PQueue* buffer, void* ptr);
int pq_empty(struct PQueue* buffer);
void pq_clear(struct PQueue* buffer);	// clears buffer (sets first = size = 0)

#endif
