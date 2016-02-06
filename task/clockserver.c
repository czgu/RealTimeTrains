#include <clockserver.h>

#include <priority.h>
#include <syscall.h>
#include <bwio.h>
#include <ts7200.h>

#define TIMER_INIT_VAL 5084 //69

void clock_init() {
    unsigned int timer_base = TIMER3_BASE;
    volatile int* timer_loader = (int*)(timer_base + LDR_OFFSET);
    volatile int* timer_control = (int*)(timer_base + CTRL_OFFSET);

    *timer_loader = TIMER_INIT_VAL;
    *timer_control = CLKSEL_MASK | MODE_MASK | ENABLE_MASK;
}

void clockserver_init(Wait_Queue* wait_queue) {
    wait_queue_init(wait_queue);

    RegisterAs("Clock Server");
    Create(CLOCKNOTIFIER_PRIORITY, clocknotifier_task);

    clock_init();
}

void clockserver_task() {
    Wait_Queue wait_queue;

    clockserver_init(&wait_queue);

    unsigned int ticks_elapsed = 0;
    CSmsg msg;
    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(CSmsg));
        if (sz != sizeof(CSmsg)) {
        } else {
            switch(msg.opcode) {
                case UPDATE_TIME: {
                    // YOU THERE: I'm not sure if this is correct, yes it is, you need to have faith in yourself
                    ticks_elapsed++;
                    msg.err = 0;
                    Reply(sender, (void*) &msg, sizeof(CSmsg));

                    reply_expired_tasks(&wait_queue, ticks_elapsed);
                    break;
                }
                case TIME_REQUEST: {
                    msg.data = ticks_elapsed; 
                    msg.err = 0;
                    Reply(sender, (void*) &msg, sizeof(CSmsg));
                    break;
                }
                case DELAY_REQUEST:
                    msg.data += ticks_elapsed;
                    // FALL-THROUGH
                case DELAYUNTIL_REQUEST: {
                    wait_queue_push(&wait_queue, msg.data);
                    break;
                }
                default:
                    DEBUG_MSG("clockserver: unknown opcode %d\n\r", msg.opcode);
                    break;
            }
        }
        // the error will be -2, if msg.err != 0
        Reply(sender, (void*) &msg, sizeof(CSmsg));
    }
}

// Clock notifier
void clocknotifier_task() {
    int server_tid = WhoIs("Clock Server");
    CSmsg msg;
    msg.opcode = UPDATE_TIME;
    for (;;) {
        msg.data = AwaitEvent(TIMER_IRQ);
        int err = Send(server_tid, (void*) &msg, sizeof(CSmsg), 
                                   (void*) &msg, sizeof(CSmsg));
        switch(err) {
            case 0:
                // success
                break;
            case -1:
                DEBUG_MSG("clocknotifier_task (error %d): %s\n\r", err,
                          "clock server id is impossible");
                break;
            case -2:
                DEBUG_MSG("clocknotifier_task (error %d): %s\n\r", err,
                          "clock server id is not an existing task");
                break;
            case -3:
                DEBUG_MSG("clocknotifier_task (error %d): %s\n\r", err,
                          "send-receive-reply transaction is incomplete");
                break;
            default:
                // unknown error code
                DEBUG_MSG("clocknotifier_task (error %d): %s\n\r", err,
                          "unknown error");
                break;
        }
    }
}

void wait_queue_init(Wait_Queue* wq) {
    pq_init(&wq->free_pool);
    int i;
    for (i = 0; i < WAIT_QUEUE_SIZE; i++) {
        pq_push(&wq->free_pool, &wq->buffer[i]);
    }

    wq->head = 0;
}

int wait_queue_push(Wait_Queue* wq, Wait_Task* task) {
    Wait_Task* elem = pq_pop(&wq->free_pool);
    *elem = *task;
    elem->next = 0;

    Wait_Task** next_node = &wq->head;
    while (1) {
        if (*next_node == 0 || (*next_node)->time > elem->time) {
            elem->next = *next_node;
            *next_node = elem;
            break;
        }     
        next_node = &((*next_node)->next);
    }
    return 0;
}

void reply_expired_tasks(Wait_Queue* wq, int time) {
    Wait_Task** node = &wq->head;
    while (1) {
        if (*node == 0) {
            return;
        } else if ((*node)->time <= time) {
            Reply((*node)->tid, 0, 0);
            pq_push(&(wq->free_pool), *node); 
            *node = (*node)->next;
        } else {
            return;
        }
    }   
}
