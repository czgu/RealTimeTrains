#ifndef _K_SYSCALL_H_
#define _K_SYSCALL_H_

#include <syscall.h>
#include <task.h>

#define RETURN_ACTIVE(n) (return_to_task(n,task_scheduler->active, task_scheduler));return

void handle(Request* request, Task_Scheduler* task_scheduler);
void handle_swi(Request* request, Task_Scheduler* task_scheduler);
void handle_irq(Task_Scheduler* task_scheduler);
void return_to_task(int ret,Task* task, Task_Scheduler* task_scheduler);

// K1
void k_create(unsigned int priority, void (*code)(), Task_Scheduler* task_scheduler);
void k_tid(Task_Scheduler* task_scheduler);
void k_pid(Task_Scheduler* task_scheduler);
void k_pass(Task_Scheduler* task_scheduler);
void k_exit(Task_Scheduler* task_scheduler);

// K2
void k_send(unsigned int tid, Task_Scheduler* task_scheduler);
void k_receive(Task_Scheduler* task_scheduler);
void k_reply(unsigned int tid, Task_Scheduler* task_scheduler);

// K3
void k_awaitevent(int eventType, Task_Scheduler* task_scheduler);

#endif
