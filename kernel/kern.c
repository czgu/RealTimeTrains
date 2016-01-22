#include <bwio.h>
#include <task.h>
#include <pqueue.h>
#include <memory.h>

#include <k_syscall.h>
#include <user_task.h>

#define FOREVER for(;;)

void kernel_init(Task_Scheduler* scheduler);
void kernel_activate(Task* active, Request* request);

// ASM
extern void swi_kern_entry();
extern void swi_kern_exit();

int main() {
    Task_Scheduler task_scheduler;
    Request request;

    kernel_init(&task_scheduler);
    bwprintf(COM2, "Before Loop\n\r");

    FOREVER {
        bwprintf(COM2, "Top Of Loop\n\r");

        Task* active = scheduler_next(&task_scheduler);
        if (active == 0) {
            break;
        }

        bwprintf(COM2, "Active Task %d\n\r", active->lr);

        kernel_activate(active, &request);
        handle(&request, &task_scheduler);
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
    void (*code)() = &first_task;
    k_create(MED, code, task_scheduler, 0);
}

void kernel_activate(Task* active, Request* request) {
    request->opcode = NONE;
    bwprintf(COM2, "check point 4 %d\n\r", active->lr);
    swi_kern_exit();
    bwprintf(COM2, "check point 5\n\r");
}
