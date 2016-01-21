#ifndef BQUEUE_H
#define BQUEUE_H

// TODO: figure out appropriate value for capacity
// Capacity size is completely arbitrary
#define CAPACITY 500

// access is not mutexed

struct BQueue {
	//const unsigned int capacity = CAPACITY;
	unsigned int size;		// size of buffer
	unsigned int first; 	// index of first element
	
	char buffer[CAPACITY];

};

void bq_init(struct BQueue* buffer);
char bq_first(struct BQueue* buffer);
char bq_pop(struct BQueue* buffer);
char bq_pop_back(struct BQueue* buffer);
int bq_push(struct BQueue* buffer, char c);
int bq_empty(struct BQueue* buffer);
void bq_clear(struct BQueue* buffer);	// clears buffer (sets first = size = 0)

#endif
