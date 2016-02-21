#ifndef BQUEUE_H
#define BQUEUE_H

// TODO: figure out appropriate value for capacity
// Capacity size is completely arbitrary
#define CAPACITY 200

// access is not mutexed

typedef struct BQueue {
	//const unsigned int capacity = CAPACITY;
	unsigned int size;		// size of buffer
	unsigned int first; 	// index of first element
	
	char buffer[CAPACITY];

} BQueue;

void bq_init(BQueue* buffer);
char bq_first(BQueue* buffer);
char bq_pop(BQueue* buffer);
char bq_pop_back(BQueue* buffer);
int bq_push(BQueue* buffer, char c);
int bq_empty(BQueue* buffer);
void bq_clear(BQueue* buffer);	// clears buffer (sets first = size = 0)
void bq_dump(BQueue* buffer, char* c);

#endif
