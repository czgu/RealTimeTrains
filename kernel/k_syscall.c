#include <k_syscall.h>

#include <bwio.h>
#include <memory.h>
#include <string.h>
#include <task.h>

void handle(Request* request, Task_Scheduler* task_scheduler) {
    unsigned int* param = request->param;
    task_scheduler->active->last_request = request;

    DEBUG_MSG("handle syscall: %d\n\r", request->opcode);

    switch (request->opcode) {
    case CREATE:
        k_create(param[0], (void (*)())param[1], task_scheduler);
        break;
    case TID:
        k_tid(task_scheduler);
        break;
    case PID:
        k_pid(task_scheduler);
        break;
    case PASS:
        k_pass(task_scheduler);
        break;
    case EXIT:
        k_exit(task_scheduler);
        break;
    case SEND:
        k_send(param[0], task_scheduler);
        break;
    case RECEIVE:
        k_receive(task_scheduler);
        break;
    case REPLY:
        k_reply(param[0], task_scheduler);
        break;
    default:
        bwprintf(COM2, "Invalid syscall");
        break;
    }
}

void return_to_task(int ret, Task* task, Task_Scheduler* task_scheduler) {
    // when kernel calls
    if (task == 0)
        return;

    task->ret = ret;
    scheduler_push(task_scheduler, task);
}


void k_create(TASK_PRIORITY priority, void (*code)(), Task_Scheduler* task_scheduler) {
    if (priority > TOP || priority < LOW) {
        RETURN_ACTIVE(-1);
    }

    Task* new_task = 0;
    if (scheduler_pop_free_task(task_scheduler, &new_task) != 0) {
        RETURN_ACTIVE(-2);
    }
    unsigned short index,gen;
    split_tid(new_task->tid, &index, &gen);

    new_task->tid = merge_tid(index, gen + 1);
    new_task->pid = task_scheduler->active->tid;
    new_task->lr = (unsigned int)code;
    new_task->ret = 0;
    // new_task->state = READY; set when added to scheduler
    new_task->priority = priority;
    new_task->spsr = 0x10;

    // Initialize program memory
    unsigned int *sp = (unsigned int*)TASK_BASE_SP + index * TASK_STACK_SIZE;
    sp[0] = (unsigned int)Exit; // lr
    sp[-2] = (unsigned int)sp; // fp

    int i;
    for(i = -3; i >= -9; i--) {
        sp[i] = 0;
    }

    new_task->sp = (int) (sp - 9); // {r4-r12, lr}
    
    scheduler_push(task_scheduler, new_task);

    bwprintf(COM2, "Created: %d\n\r", new_task->tid);
    
    RETURN_ACTIVE(0);
}

void k_tid(Task_Scheduler* task_scheduler) {
    RETURN_ACTIVE(task_scheduler->active->tid);
}

void k_pid(Task_Scheduler* task_scheduler) {
    RETURN_ACTIVE(task_scheduler->active->pid);
}

void k_pass(Task_Scheduler* task_scheduler) {
    // do nothing
    RETURN_ACTIVE(0);
}

void k_exit(Task_Scheduler* task_scheduler) {
    scheduler_push_free_task(task_scheduler, task_scheduler->active);
}

// helpers ----------------------------------------

void send_message(Task* receiver, Task* sender, Task_Scheduler* task_scheduler) {
    Request* sender_request = sender->last_request;
    Request* receiver_request = receiver->last_request;

    int size_copied = memory_copy(
        (void *)sender_request->param[1],
        sender_request->param[2],
        (void *)receiver_request->param[1],
        receiver_request->param[2]
    );

    *((int *)receiver_request->param[0]) = sender->tid;

    // sender is replied block
    sender->state = REPLY_BLOCKED;
    // receiver is made ready again
    return_to_task(size_copied, receiver, task_scheduler);    
}

// -----------------------------------------------

void k_send(unsigned int tid, Task_Scheduler* task_scheduler) {
    Task* sender = task_scheduler->active;

    unsigned short receiver_index, receiver_gen;
    split_tid(tid, &receiver_index, &receiver_gen);
    
    if (receiver_index >= TASK_POOL_SIZE) {
        RETURN_ACTIVE(-1);
    }

    Task* receiver = &task_scheduler->task_pool[receiver_index];
    // task replaced, or not yet created
    if (receiver->tid != tid ) {
        RETURN_ACTIVE(-2);
    }

    switch(receiver->state) {
    case ZOMBIE:
        RETURN_ACTIVE(-2); // task ended
        break;
    case SEND_BLOCKED:
        send_message(receiver, sender, task_scheduler);
        break;
    default:
        send_queue_push(receiver, sender);
        break;    
    }
}

void k_receive(Task_Scheduler* task_scheduler) {
    Task* sender;
    Task* receiver = task_scheduler->active;

    if (send_queue_pop(receiver, &sender) == 0) {
        send_message(receiver, sender, task_scheduler);
    } else {
        receiver->state = SEND_BLOCKED;
    }
}

void k_reply(unsigned int tid, Task_Scheduler* task_scheduler) {
    Task* receiver = task_scheduler->active;

    unsigned short sender_index, sender_gen;
    split_tid(tid, &sender_index, &sender_gen);

    // invalid reply to tid
    if (sender_index >= TASK_POOL_SIZE) {
        RETURN_ACTIVE(-1); 
    }

    Task* sender = &task_scheduler->task_pool[sender_index];

    // task replaced or not yet created
    if (sender->tid != tid) {
        RETURN_ACTIVE(-2);
    }

    Request* sender_request = sender->last_request;
    Request* receiver_request = receiver->last_request;

    // sender isn't reply blocked or waiting on reply from this receiver
    if (sender->state != REPLY_BLOCKED || 
        sender_request->param[0] != receiver->tid) {
        RETURN_ACTIVE(-3); 
    }

    // if insufficient copy space
    if (receiver_request->param[2] > sender_request->param[4]) {
        RETURN_ACTIVE(-4);
    }

    int size_copied = memory_copy(
        (void *)receiver_request->param[1],
        receiver_request->param[2],
        (void *)sender_request->param[3],
        sender_request->param[4]
    );

    // sender is made ready again (it's made ready first)
    return_to_task(size_copied, sender, task_scheduler);    
    // receiver is made ready again (not sure what return value yet)
    RETURN_ACTIVE(0);
}
