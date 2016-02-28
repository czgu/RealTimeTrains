#include <clockserver.h>

#include <priority.h>
#include <syscall.h>
#include <bwio.h>
#include <ts7200.h>

void clockserver_init(Wait_Queue* wait_queue) {
    wait_queue_init(wait_queue);

    RegisterAs("Clock Server");
    Create(CLOCKNOTIFIER_PRIORITY, clocknotifier_task);
}

void clockserver_task() {
    Wait_Queue wait_queue;

    clockserver_init(&wait_queue);

    unsigned int ticks_elapsed = 0;
    CSmsg msg;
    Wait_Task wait_task;
    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(CSmsg));
        if (sz == sizeof(CSmsg)) {
            switch(msg.opcode) {
                case UPDATE_TIME: {
                    // TODO: verify if this would lose ticks
                    ticks_elapsed++;
                    msg.err = 0;

                    //DEBUG_MSG("timer update %d\n\r", ticks_elapsed);

                    Reply(sender, 0, 0);
                    reply_expired_tasks(&wait_queue, ticks_elapsed);
                    break;
                }
                case TIME_REQUEST: {
                    msg.data = ticks_elapsed; 
                    msg.err = 0;
                    
                    // DEBUG_MSG("reply to time request %d\n\r", sender);

                    Reply(sender, (void*) &msg, sizeof(CSmsg));
                    break;
                }
                case DELAY_REQUEST:
                    msg.data += ticks_elapsed;
                    // FALL-THROUGH
                case DELAYUNTIL_REQUEST: {
                    // DEBUG_MSG("added to wait queue%d\n\r", sender);

                    wait_task.tid = sender;
                    wait_task.time = msg.data; 

                    wait_queue_push(&wait_queue, &wait_task);
                    break;
                }
                default:
                    // DEBUG_MSG("clockserver: unknown opcode %d\n\r", msg.opcode);
                    break;
            }
        }
    }
}

// Clock notifier
void clocknotifier_task() {
    int server_tid = WhoIs("Clock Server");
    CSmsg msg;
    msg.opcode = UPDATE_TIME;
    for (;;) {
        msg.data = AwaitEvent(TIMER_IRQ);
        Send(server_tid, (void*) &msg, sizeof(CSmsg), 0, 0);
        /*
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
        */
    }
}

void wait_queue_init(Wait_Queue* wq) {
    pq_init(&wq->free_pool);
    int i;
    for (i = 0; i < WAIT_QUEUE_SIZE; i++) {
        wq->buffer[i].time = -1;    // signed to unsigned = large number
        wq-> buffer[i].tid = 0;
        pq_push(&wq->free_pool, &wq->buffer[i]);
    }

    wq->head = 0;
}

int wait_queue_push(Wait_Queue* wq, Wait_Task* task) {
    Wait_Task* elem = pq_pop(&wq->free_pool);
    if (elem == 0) {
        return -1;
    }
    *elem = *task;

    if (wq->head == 0 || elem->time < wq->head->time) {
        wq->head = elem;
    }
    // DEBUG_MSG("wait queue head %d %d %d\n\r", (int)(wq->head), elem->time, task->time);
    return 0;
}

void reply_expired_tasks(Wait_Queue* wq, int time) {
    if (wq->head == 0 || time < wq->head->time) {
        return;
    }
    int i;
    wq->head = 0;
    for (i = 0; i < WAIT_QUEUE_SIZE; i++) {
        if (wq->buffer[i].time <= time) {
            // DEBUG_MSG("remove wait queue %d %d\n\r",time, (*node)->tid);
            Reply(wq->buffer[i].tid, 0, 0);

            // reset cell to indicate empty
            wq->buffer[i].time = -1;
            wq->buffer[i].tid = 0;
            pq_push(&wq->free_pool, &wq->buffer[i]);
        } else if (wq->head == 0 || wq->buffer[i].time < wq->head->time) {
            wq->head = &wq->buffer[i];
        }
    }
}
