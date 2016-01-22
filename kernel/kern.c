#include <bwio.h>
#include <task.h>
#include <pqueue.h>
#include <memory.h>

#define FOREVER for(;;)

void kernel_init();
Task* schedule(PQueue* task_queue);
void kernel_exit(Task* active);
void handle();

// ASM
extern void swi_kern_entry();
extern void swi_kern_exit();
extern void swi_jump();

void first_task() {
    register int r0 asm("r0");

    bwprintf(COM2, "~~~I'm a barbie girl with %d teddy bears <3\n\r", r0);

    swi_jump();

    bwprintf(COM2, "~~~in the barbie world, life is fantastic\n\r", r0);
}

int main() {
    Task task_pool[TASK_POOL_SIZE];
    // TODO: create a queue for each priority
    PQueue task_queue;

    kernel_init(task_pool, &task_queue);
    bwprintf(COM2, "Before Loop\n\r");

    FOREVER {
        bwprintf(COM2, "Top Of Loop\n\r");
        Task* active = schedule(&task_queue);
        if (active == 0) {
            break;
        }
        bwprintf(COM2, "Active Task %d\n\r", active->lr);
        kernel_exit(active);

        pq_push(&task_queue, (void*)active);
        // handle( tds, req );
    }
    
    return 0;
}

void kernel_init(Task* task_pool, PQueue* task_queue) {
    // initialize io
    bwsetfifo(COM2, OFF);
    bwsetspeed(COM2, 115200);
    
    // initialize low memory
    unsigned int* swi_jump_table = (unsigned int*)SWI_JUMP_TABLE;
    *swi_jump_table = (unsigned int)&swi_kern_entry;

    // initialize ICU

    // initialize task
    init_task_pool(task_pool, TASK_POOL_SIZE);
    pq_init(task_queue);

    // find space for first task
    Task* td = 0;
    unsigned int tid;
    get_free_task(task_pool, TASK_POOL_SIZE, &td, &tid);

    // initialize first task
    void (*code)() = &first_task;

    td->tid = tid;
    td->pid = 0;
    td->lr = (unsigned int)code;
    td->ret = 23;
    td->state = READY;
    td->priority = LOW;
    
    pq_push(task_queue, (void*)td);
}

Task* schedule(PQueue* task_queue) {
    if (!pq_empty(task_queue)) {
        return (Task*) pq_pop(task_queue);
    }
    return 0;
}

void kernel_exit(Task* active) {
    bwprintf(COM2, "check point 4 %d\n\r", active->lr);
    //dummy();
    swi_kern_exit();
    bwprintf(COM2, "check point 5\n\r");

    
}
