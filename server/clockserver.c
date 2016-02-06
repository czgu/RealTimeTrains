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

void clockserver_init() {
    // TODO: initialize clock data structures
    RegisterAs("Clock Server");
    Create(CLOCKNOTIFIER_PRIORITY, clocknotifier_task);
    clock_init();
}

void clockserver_task() {
    clockserver_init();

    unsigned int ticks_elapsed = 0;
    CSmsg msg;
    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(CSmsg));
        if (sz != sizeof(CSmsg)) {
        } else {
            switch(msg.opcode) {
                // TODO: handle messages
                case UPDATE_TIME: {
                    // FIXME: I'm not sure if this is correct
                    ticks_elapsed++;
                    msg.err = 0;
                    Reply(sender, (void*) &msg, sizeof(CSmsg));

                    // TODO: check wait queue for expired tasks
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
                    // TODO: add task id and msg.data to wait queue
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
