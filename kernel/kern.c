#include <bwio.h>
#include <task.h>
#include <pqueue.h>

#define FOREVER for(;;)

void kernel_init();
Task* schedule(PQueue* task_queue);
void kernel_exit(Task* active);
void handle();

void first_task() {
    bwprintf(COM2, "I am a barbie girl, live in a plastic world\n\r");
}


int main() {
    Task task_pool[TASK_POOL_SIZE];
    PQueue task_queue;

    kernel_init(task_pool, &task_queue);

    FOREVER {
        Task* active = schedule(&task_queue);
        kernel_exit(active);
        // handle( tds, req );
    }
    
    return 0;
}

void kernel_init(Task* task_pool, PQueue* task_queue) {
    // initialize io
    bwsetfifo(COM2, OFF);

    // initialize task
    init_task_pool(task_pool, TASK_POOL_SIZE);
    pq_init(task_queue);
    // add first task

    Task* td = 0;
    unsigned int tid;
    get_free_task(task_pool, TASK_POOL_SIZE, &td, &tid);
    
    void (*code)();
    code = first_task; 
    // initialize first task
    td->tid = tid;
    td->pid = 0;
    td->sp = 0x00300000;
    td->lr = (unsigned int)code;
    td->spsr = 0x10;
    td->ret = 0;
    td->state = READY;
    td->priority = LOW;
    
    pq_push(task_queue, (void*)td);
}

Task* schedule(PQueue* task_queue) {
    if (!pq_empty(task_queue)) {
        return (Task*)pq_pop(task_queue);
    }

    return 0;
}

void kernel_exit(Task* active) {
    
}
