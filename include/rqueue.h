#ifndef _RQUEUE_H_
#define _RQUEUE_H_

// Ring Queue, can hold any array of any size, need to tell type before start
// the popped element are pointers and are deferenced by the caller
typedef struct RQueue {
	unsigned int size;		// size
	unsigned int first; 	// index of first element
	
    unsigned int max_size;
    unsigned int unit_size;
    unsigned int buffer;
} RQueue;

void rq_init(RQueue* q, void* buffer, unsigned int max_size, unsigned unit_size);
inline void* rq_first(RQueue* q);
inline void* rq_pop_back(RQueue* q);
void* rq_pop_front(RQueue* q);
int rq_push_back(RQueue* q, void* ptr);
int rq_push_front(RQueue* q, void* ptr);
inline int rq_empty(RQueue* q);
inline void rq_clear(RQueue* q);	// clears buffer (sets first = size = 0)
inline void* rq_get(RQueue* q, int index);

#endif
