#include <task.h>

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
        tasks[i].state = T_ZOMBIE;
        tasks[i].tid = 0;
    }
}

int get_free_task(Task* tasks, int size, Task** free_task, unsigned int* tid) {
    int i;
    for (i = 0; i < size; i++) {
        if (tasks[i].state == T_ZOMBIE) {
            unsigned short index, gen;
            split_tid(tasks[i].tid, &index, &gen);
            *tid = merge_tid(index, gen);
            
            *free_task = &tasks[i];

            return 0;
        }
    }
    return -1;
}
