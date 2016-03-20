#include <bwio.h>
#include <task.h>
#include <memory.h>
#include <assert.h>

inline unsigned int ctz(unsigned int v) {
    unsigned int c;     // c will be the number of zero bits on the right,
                        // so if v is 1101000 (base 2), then c will be 3
    // NOTE: if 0 == v, then c = 31.
    if (v & 0x1) 
    {
      // special case for odd v (assumed to happen half of the time)
      c = 0;
    }
    else
    {
      c = 1;
      if ((v & 0xffff) == 0) 
      {  
        v >>= 16;  
        c += 16;
      }
      if ((v & 0xff) == 0) 
      {  
        v >>= 8;  
        c += 8;
      }
      if ((v & 0xf) == 0) 
      {  
        v >>= 4;
        c += 4;
      }
      if ((v & 0x3) == 0) 
      {  
        v >>= 2;
        c += 2;
      }
      c -= v & 0x1;
    }
    return c;
}

// TASK STRUCT

void split_tid(unsigned int tid, unsigned short* index, unsigned short* generation) {
    *index = (unsigned short) tid;
    *generation = (unsigned short) (tid >> 16);
}

unsigned int merge_tid(unsigned short index, unsigned short generation) {
    return (unsigned int)((generation << 16) | index);
}

void send_queue_push(Task* receiver, Task* sender) {
    ASSERT(receiver->state != SEND_BLOCKED);
    sender->state = RECEIVE_BLOCKED;

    // for safety, may not be necessary
    sender->send_queue_next = 0;

    if (receiver->send_queue_next == 0) {
        receiver->send_queue_next = sender;
    } else {
        receiver->send_queue_last->send_queue_next = sender;
    }
    receiver->send_queue_last = sender;
}

int send_queue_pop(Task* receiver, Task** sender) {
    ASSERT(receiver->state != SEND_BLOCKED);

    if (receiver->send_queue_next == 0) { 
        return -1; // send_queue_empty       
    } else {
        // find the sender
        *sender = receiver->send_queue_next;

        // update the send_queue to find the next element
        receiver->send_queue_next = receiver->send_queue_next->send_queue_next;

        if (receiver->send_queue_next == 0) {
            // we just popped the last element, not sure if this is necessary
            receiver->send_queue_last = 0; 
        }

        // clear the send chain for receiver as well
        (*sender)->send_queue_next = 0;
    }
    return 0;
}

// EVENTS
void events_init(Event* events) {
    int i;
    for (i = 0; i < EVENT_FLAG_LEN; i++) {
        events[i].wait_task = 0;
    }
}

// TASK_SCHEDULER
void scheduler_init(Task_Scheduler* scheduler) {
    int i;

    scheduler->priority_bitmap = 0;
    for (i = 0; i < TASK_NPRIORITIES; i++) {
        pq_init(scheduler->ready_queue + i);
    }

    pq_init(&scheduler->free_list);
    for (i = 0; i < TASK_POOL_SIZE; i++) {
        scheduler->task_pool[i].tid = i;
        scheduler->task_pool[i].pid = 0;           // need to be set later

        scheduler->task_pool[i].state = ZOMBIE;
        scheduler->task_pool[i].priority = 30;

        scheduler->task_pool[i].send_queue_next = 0;
        scheduler->task_pool[i].send_queue_last = 0;

        scheduler->task_pool[i].last_request = 0;

        pq_push(&scheduler->free_list, (void*)(scheduler->task_pool + i));
    }

    events_init(scheduler->events);
    scheduler->active = (Task*)0;
    scheduler->halt = 0;
}

Task* scheduler_next(Task_Scheduler* scheduler) {
    scheduler->active = (void*)0;
    if (scheduler_empty(scheduler) || scheduler->halt == 1) {
        return 0;
    }
    unsigned int priority = ctz(scheduler->priority_bitmap);

    scheduler->active = (Task*) pq_pop(scheduler->ready_queue + priority);
    scheduler->active->state = ACTIVE;

    if (pq_empty(scheduler->ready_queue + priority)) {
        scheduler->priority_bitmap &= ~(0x1u << priority);
    }

    return scheduler->active;
}

int scheduler_push(Task_Scheduler* scheduler, Task* task) {
    if (task == 0) {
        return -1;
    }

    task->state = READY;

    PQueue* pq = scheduler->ready_queue + task->priority;
    int err = pq_push(pq, (void*) task);
    if (err) {
        return err;
    }
    scheduler->priority_bitmap |= 0x1u << task->priority;
    return 0;
}

inline int scheduler_empty(Task_Scheduler* scheduler) {
    return scheduler->priority_bitmap == 0;
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

