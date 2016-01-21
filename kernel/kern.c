#include <bwio.h>
#include <task.h>
#include <pqueue.h>

#define FOREVER for(;;)

void kernel_init();
Task* schedule(PQueue* task_queue);
void kernel_exit(Task* active);
void handle();

void first_task() {
    bwprintf(COM2, "I'm a barbie girl\n\r");
}

int main() {
    Task task_pool[TASK_POOL_SIZE];
    // TODO: create a queue for each priority
    PQueue task_queue;

    kernel_init(task_pool, &task_queue);

    FOREVER {
        Task* active = schedule(&task_queue);
        if (active == 0) {
            break;
        }
        kernel_exit(active);
        // handle( tds, req );
    }
    
    return 0;
}

void kernel_init(Task* task_pool, PQueue* task_queue) {
    // initialize io
    bwsetfifo(COM2, OFF);
    bwsetspeed(COM2, 115200);
    
    // initialize low memory

    // initialize ICU

    // initialize task
    init_task_pool(task_pool, TASK_POOL_SIZE);
    pq_init(task_queue);

    // find space for first task
    Task* td = 0;
    unsigned int tid;
    get_free_task(task_pool, TASK_POOL_SIZE, &td, &tid);

    // initialize first task
    void (*code)() = first_task;

    td->tid = tid;
    td->pid = 0;
    td->lr = (unsigned int)code;
    td->ret = 0;
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
    
}
