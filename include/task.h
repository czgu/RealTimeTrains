#ifndef _TASK_H_
#define _TASK_H_

#define T_READY  0
#define T_ACTIVE 1
#define T_ZOMBIE 2

struct Task {
    int tid, pid;
    int sp, lr, spsr;
    int ret;
    int state;
    int priority;
};

#endif
