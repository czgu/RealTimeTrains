#ifndef _TASK_H_
#define _TASK_H_

#define T_READY  0
#define T_ACTIVE 1
#define T_ZOMBIE 2

#define TASK_NPRIORITIES 4
typedef enum {
    TOP = 0,
    HIGH,
    MED,
    LOW,
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

void init_task_pool(Task* tasks, int size);
int get_free_task(Task* tasks, int size, Task** free_task, unsigned int* tid);

#include <pqueue.h>

void scheduler_init(PQueue* ready_task_table);
Task* scheduler_next(PQueue* ready_task_table);
int scheduler_push(PQueue* ready_tasks_table, Task*);
int scheduler_empty(PQueue* ready_tasks_table);

#endif
