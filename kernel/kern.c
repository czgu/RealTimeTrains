#include <bwio.h>
#include <task.h>
#include <pqueue.h>
#include <memory.h>

#include <k_syscall.h>
#include <user_task.h>

#define FOREVER for(;;)

void kernel_init(Task_Scheduler* scheduler);
void kernel_activate(Task* active, Request** request);

// ASM
extern void swi_kern_entry();
extern void swi_kern_exit(Task* active, Request** request);

int main() {
    Task_Scheduler task_scheduler;
    Request* request;

    kernel_init(&task_scheduler);

    FOREVER {
        scheduler_next(&task_scheduler);
        if (task_scheduler.active == 0) {
            break;
        }

        kernel_activate(task_scheduler.active, &request);
        handle(request, &task_scheduler);
    }
    
    return 0;
}

void kernel_init(Task_Scheduler* task_scheduler) {
    // initialize io
    bwsetfifo(COM2, OFF);
    bwsetspeed(COM2, 115200);
    
    // initialize low memory
    unsigned int* swi_jump_table = (unsigned int*)SWI_JUMP_TABLE;
    *swi_jump_table = (unsigned int)&swi_kern_entry;

    // initialize ICU

    // initialize scheduler
    scheduler_init(task_scheduler);

    // initialize first task
    k_create(MED, (void *)first_task, task_scheduler, 0);
}

void kernel_activate(Task* active, Request** request) {
    swi_kern_exit(active, request);
}
