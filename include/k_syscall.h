#include <syscall.h>
#include <task.h>

typedef struct Request {
    SYSCALL opcode;
    unsigned int param[10];
} Request;

void handle(Request* request, Task_Scheduler* task_scheduler);
int k_create(TASK_PRIORITY priority, void (*code)(), Task_Scheduler* task_scheduler, unsigned int pid);
int k_tid(Task_Scheduler* task_scheduler);
int k_pid(Task_Scheduler* task_scheduler);
void k_pass(Task_Scheduler* task_scheduler);
void k_exit(Task_Scheduler* task_scheduler);
