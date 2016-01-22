#include <task.h>
#include <memory.h>

void split_tid(unsigned int tid, unsigned short* index, unsigned short* generation) {
    *index = (unsigned short) tid;
    *generation = (unsigned short) (tid >> 16);
}

unsigned int merge_tid(unsigned short index, unsigned short generation) {
    return (unsigned int)((generation << 16) | index);
}

void scheduler_init(Task_Scheduler* scheduler) {
    int i;
    for (i = LOW; i <= TOP; i++) {
        pq_init(scheduler->ready_queue + i);
    }

    pq_init(&scheduler->free_list);
    for (i = 0; i < TASK_POOL_SIZE; i++) {
        scheduler->task_pool[i].tid = i;
        scheduler->task_pool[i].pid = 0;           // need to be set later

        scheduler->task_pool[i].state = T_ZOMBIE;
        scheduler->task_pool[i].priority = LOW;

        pq_push(&scheduler->free_list, (void*)(scheduler->task_pool + i));
    }

    scheduler->active = (Task*)0;
}

Task* scheduler_next(Task_Scheduler* scheduler) {
    int i;
    for (i = TOP; i >= LOW ; i--) {
        if (!pq_empty(scheduler->ready_queue + i)) {
            scheduler->active = (Task*) pq_pop(scheduler->ready_queue + i);
            return scheduler->active;
        }
    }
    return (void*)0;
}

int scheduler_push(Task_Scheduler* scheduler, Task* task) {
    if (task == 0) {
        return -1;
    }
    PQueue* pq = scheduler->ready_queue + task->priority;
    return pq_push(pq, (void*) task);
}

int scheduler_empty(Task_Scheduler* scheduler) {
    int i;
    for (i = LOW; i <= TOP; i++) {
        if (!pq_empty(scheduler->ready_queue + i)) {
            return 0;
        }
    }
    return 1;
}

int scheduler_pop_free_task(Task_Scheduler* scheduler, Task** free_task) {
    if (!pq_empty(&scheduler->free_list)) {
        // assert task->state == T_ZOMBIE?
        *free_task = (Task*)pq_pop(&scheduler->free_list);
        return 0;
    }
    return -1;
}

int scheduler_push_free_task(Task_Scheduler* scheduler, Task* task) {
    // assert task->state == T_ZOMBIE?
    // task == 0?
    if (task == (void*)0) {
        return -1;
    }

    return pq_push(&scheduler->free_list, task);
}

