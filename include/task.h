#ifndef _TASK_H_
#define _TASK_H_

#include <pqueue.h>

#define T_READY  0
#define T_ACTIVE 1
#define T_ZOMBIE 2

#define TASK_NPRIORITIES 4
typedef enum {
    LOW = 0,
    MED,
    HIGH,
    TOP,
} TASK_PRIORITY;

typedef enum {
    READY = 0,
    ACTIVE,
    ZOMBIE,
} TASK_STATE;

typedef struct Task {
    // kernel has tid = 0; all other tid's start at 1
    unsigned int tid; // #0
    unsigned int pid; // #4
    unsigned int sp;  // #8
    unsigned int lr;  // #12
    unsigned int spsr; // #16
    int ret; // #20
    TASK_STATE state; // #24
    TASK_PRIORITY priority; // #28
} Task;

void split_tid(unsigned int tid, unsigned short* index, unsigned short* generation);
unsigned int merge_tid(unsigned short index, unsigned short generation);

#define TASK_POOL_SIZE  60

typedef struct Task_Scheduler {
    Task task_pool[TASK_POOL_SIZE];
    PQueue free_list;
    PQueue ready_queue[TASK_NPRIORITIES];
    Task* active;
} Task_Scheduler;

void scheduler_init(Task_Scheduler* scheduler);
Task* scheduler_next(Task_Scheduler* scheduler);
int scheduler_push(Task_Scheduler* scheduler, Task* task);
int scheduler_empty(Task_Scheduler* scheduler);
int scheduler_pop_free_task(Task_Scheduler* scheduler, Task** free_task);
int scheduler_push_free_task(Task_Scheduler* scheduler, Task* task);



#endif
