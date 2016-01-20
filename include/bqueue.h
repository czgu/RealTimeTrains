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

void init(struct BQueue* buffer);
char first(struct BQueue* buffer);
char pop(struct BQueue* buffer);
char pop_back(struct BQueue* buffer);
int push(struct BQueue* buffer, char c);
int empty(struct BQueue* buffer);
void clear(struct BQueue* buffer);	// clears buffer (sets first = size = 0)

#endif
