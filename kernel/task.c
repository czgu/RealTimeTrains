#include <task.h>
#include <memory.h>

void split_tid(unsigned int tid, unsigned short* index, unsigned short* generation) {
    *index = (unsigned short) tid;
    *generation = (unsigned short) (tid >> 16);
}

unsigned int merge_tid(unsigned short index, unsigned short generation) {
    return (unsigned int)((generation << 16) | index);
}

void init_task_pool(Task* tasks, int size) {
    int i;
    for (i = 0; i < size; i++) {
        tasks[i].tid = i + 1;
        tasks[i].pid = 0;           // need to be set later

        tasks[i].sp = TASK_BASE_SP + i * TASK_STACK_SIZE;
        tasks[i].spsr = 0x10;

        tasks[i].state = T_ZOMBIE;
        tasks[i].priority = LOW;
    }
}

int get_free_task(Task* tasks, int size, Task** free_task, unsigned int* tid) {
    int i;
    // TODO: replace linear search for free task with list of free tasks
    for (i = 0; i < size; i++) {
        if (tasks[i].state == T_ZOMBIE) {
            unsigned short index, gen;
            split_tid(tasks[i].tid, &index, &gen);
            *tid = merge_tid(index, gen + 1);
            
            *free_task = &tasks[i];

            return 0;
        }
    }
    return -1;
}

void scheduler_init(PQueue* priorities) {
    int i;
    for (i = 0; i < TASK_NPRIORITIES; i++) {
        pq_init(priorities + i);
    }
}

Task* scheduler_next(PQueue* priorities) {
    int i;
    for (i = 0; i < TASK_NPRIORITIES; i++) {
        if (!pq_empty(priorities + i)) {
            return (Task*) pq_pop(priorities + i);
        }
    }
    return 0;
}

int scheduler_push(PQueue* priorities, Task* task) {
    if (task == 0) {
        return -1;
    }
    PQueue* pq = priorities + task->priority;
    return pq_push(pq, (void*) task);
}

int scheduler_empty(PQueue* priorities) {
    int i;
    for (i = 0; i < TASK_NPRIORITIES; i++) {
        if (!pq_empty(priorities + i)) {
            return 0;
        }
    }
    return 1;
}
