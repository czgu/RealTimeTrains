#include <bwio.h>

void handle(Request* request, Task* task, PQueue* ready_task_table, Task* task_pool) {
    unsigned int* param = request->param;
    switch (request->opcode) {
    case CREATE:
        k_create(param[0], (void (*)())param[1], task, ready_task_table);
        break;
    case TID:
        k_tid(task, ready_task_table);
        break;
    case PID:
        k_pid(task, ready_task_table);
        break;
    case PASS:
        k_pass(task, ready_task_table);
        break;
    case EXIT:
        k_exit(task, ready_task_table);
        break;
    default:
        bwprintf(COM2, "Invalid syscall");
        break;
    }
}

int k_create(int priority, void (*code)(), Task* task, PQueue* ready_task_table) {
    
}

int k_tid(Task* task, PQueue* ready_task_table) {
}

int k_pid(Task* task, PQueue* ready_task_table) {
}

void k_pass(Task* task, PQueue* ready_task_table) {
}

void k_exit(Task* task, PQueue* ready_task_table) {
}
