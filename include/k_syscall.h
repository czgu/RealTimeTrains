#include <syscall.h>
#include <task.h>

struct Request {
    SYSCALL opcode;
    unsigned int param[10];
};

void handle(Request* request, Task* task, PQueue* ready_task_table, Task* task_pool);
int k_create(TASK_PRIORITY priority, void (*code)(), Task* task, PQueue* ready_task_table);
int k_tid(Task* task, PQueue* ready_task_table);
int k_pid(Task* task, PQueue* ready_task_table);
void k_pass(Task* task, PQueue* ready_task_table);
void k_exit(Task* task, PQueue* ready_task_table);
