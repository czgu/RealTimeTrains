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
} PRIORITY;

typedef struct Task {
    unsigned int tid, pid;
    unsigned int sp, lr, spsr;
    int ret;
    int state;
    PRIORITY priority;
} Task;

void split_tid(unsigned int tid, unsigned short* index, unsigned short* generation);
unsigned int merge_tid(unsigned short index, unsigned short generation);

#define TASK_POOL_SIZE 60

void init_task_pool(Task* tasks, int size);
int get_free_task(Task* tasks, int size, Task** free_task, int* tid);

#endif
