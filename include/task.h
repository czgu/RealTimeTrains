#ifndef _TASK_H_
#define _TASK_H_

#define T_READY  0
#define T_ACTIVE 1
#define T_ZOMBIE 2

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
    unsigned int tid, pid;      // kernel has tid = 0; all other tid's start at 1
    unsigned int sp, lr, spsr;
    int ret;
    TASK_STATE state;
    TASK_PRIORITY priority;
} Task;

void split_tid(unsigned int tid, unsigned short* index, unsigned short* generation);
unsigned int merge_tid(unsigned short index, unsigned short generation);

#define TASK_POOL_SIZE  60
#define TASK_BASE_SP    0x00300000
#define TASK_STACK_SIZE 524288      // 0.5 MB

void init_task_pool(Task* tasks, int size);
int get_free_task(Task* tasks, int size, Task** free_task, unsigned int* tid);

#endif
