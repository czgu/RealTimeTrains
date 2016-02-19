#ifndef _CLOCKSERVER_H_
#define _CLOCKSERVER_H_

#include <pqueue.h>

// Clock server
typedef enum {
    UPDATE_TIME = 0,
    TIME_REQUEST,
    DELAY_REQUEST,
    DELAYUNTIL_REQUEST,
} CSOP;

typedef struct CSmsg {
    CSOP opcode;
    //int type;
    unsigned int data;
    int err;
} CSmsg;

void clockserver_init();
void clockserver_task();

// Clock notifier
void clocknotifier_task();

// Wait queue
// unordered priority queue (implemented as circular array)
typedef struct Wait_Task {
    unsigned int time;
    int tid;
} Wait_Task;

#define WAIT_QUEUE_SIZE 40
typedef struct Wait_Queue {
    Wait_Task buffer[WAIT_QUEUE_SIZE];
    PQueue free_pool;
    
    Wait_Task* head;
} Wait_Queue;

void wait_queue_init(Wait_Queue* wq);
int wait_queue_push(Wait_Queue* wq, Wait_Task* task);
void reply_expired_tasks(Wait_Queue* wq, int time);

#endif
