#ifndef _TASK_H_
#define _TASK_H_

#include <pqueue.h>
#include <task_type.h>

typedef struct Task {
    // kernel has tid = 0; all other tid's start at 1
    unsigned int tid; // #0
    unsigned int pid; // #4
    unsigned int sp;  // #8
    unsigned int lr;  // #12
    unsigned int spsr; // #16
    int ret; // #20
    TASK_STATE state; // #24
    unsigned int priority; // #28

    // all task in the linked list should be receive blocked except head
    struct Task* send_queue_next; // #32
    // for queue insertion, only set for head
    struct Task* send_queue_last; // #36

    Request* last_request; //#40
} Task;

void split_tid(unsigned int tid, unsigned short* index, unsigned short* generation);
unsigned int merge_tid(unsigned short index, unsigned short generation);
void send_queue_push(Task* receiver, Task* sender);
int send_queue_pop(Task* receiver, Task** sender);

typedef struct Event {
    short index;
    Task* wait_task;
} Event;
void events_init(Event* events);


#define TASK_POOL_SIZE  40
typedef struct Task_Scheduler {
    Task task_pool[TASK_POOL_SIZE];
    PQueue free_list;
    PQueue ready_queue[TASK_NPRIORITIES];
    unsigned int priority_bitmap;
    Task* active;

    Event events[EVENT_FLAG_LEN];    
} Task_Scheduler;

void scheduler_init(Task_Scheduler* scheduler);
Task* scheduler_next(Task_Scheduler* scheduler);
int scheduler_push(Task_Scheduler* scheduler, Task* task);
inline int scheduler_empty(Task_Scheduler* scheduler);
int scheduler_pop_free_task(Task_Scheduler* scheduler, Task** free_task);
int scheduler_push_free_task(Task_Scheduler* scheduler, Task* task);



#endif
