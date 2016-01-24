#include <k_syscall.h>

#include <bwio.h>
#include <memory.h>
#include <task.h>

void handle(Request* request, Task_Scheduler* task_scheduler) {
    unsigned int* param = request->param;

    // bwprintf(COM2, "handle syscall %d \n\r", request->opcode);
    switch (request->opcode) {
    case CREATE:
        task_scheduler->active->ret =
            k_create(param[0], (void (*)())param[1], task_scheduler, task_scheduler->active->tid);
        break;
    case TID:
        task_scheduler->active->ret = k_tid(task_scheduler);
        break;
    case PID:
        task_scheduler->active->ret = k_pid(task_scheduler);
        break;
    case PASS:
        k_pass(task_scheduler);
        break;
    case EXIT:
        k_exit(task_scheduler);
        break;
    default:
        bwprintf(COM2, "Invalid syscall");
        break;
    }

    if (request->opcode != EXIT) {
        scheduler_push(task_scheduler, task_scheduler->active);
    }
}

int k_create(TASK_PRIORITY priority, void (*code)(), Task_Scheduler* task_scheduler, unsigned int pid) {
    if (priority > TOP || priority < LOW) {
        return -1;
    }

    Task* new_task = 0;
    if (scheduler_pop_free_task(task_scheduler, &new_task) != 0) {
        return -2;
    }
    unsigned short index,gen;
    split_tid(new_task->tid, &index, &gen);

    new_task->tid = merge_tid(index, gen + 1);
    new_task->pid = pid;
    new_task->lr = (unsigned int)code;
    new_task->ret = 0;
    new_task->state = READY;
    new_task->priority = priority;
    new_task->spsr = 0x10;

    // Initialize program memory
    unsigned int *sp = (int*)TASK_BASE_SP + index * TASK_STACK_SIZE;
    sp[0] = (unsigned int)Exit; // lr
    sp[-2] = (unsigned int)sp; // fp

    int i;
    for(i = -3; i >= -9; i--) {
        sp[i] = 0;
    }

    new_task->sp = (int) (sp - 9); // {r4-r12, lr}
    
    scheduler_push(task_scheduler, new_task);

    bwprintf(COM2, "Created: %d %d\n\r", new_task->tid, sp[0]);
    return 0;
}

int k_tid(Task_Scheduler* task_scheduler) {
    return task_scheduler->active->tid;
}

int k_pid(Task_Scheduler* task_scheduler) {
    return task_scheduler->active->pid;
}

void k_pass(Task_Scheduler* task_scheduler) {
    // do nothing
}

void k_exit(Task_Scheduler* task_scheduler) {
    scheduler_push_free_task(task_scheduler, task_scheduler->active);
}
