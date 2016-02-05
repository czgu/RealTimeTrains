#include <syscall.h>
#include <bwio.h>
#include <ts7200.h>

#include <clockserver.h>

#define TIMER_INIT_VAL 5084 //69

void clock_init() {
    unsigned int timer_base = TIMER3_BASE;
    volatile int* timer_loader = (int*)(timer_base + LDR_OFFSET);
    volatile int* timer_control = (int*)(timer_base + CTRL_OFFSET);

    *timer_loader = TIMER_INIT_VAL;
    *timer_control = CLKSEL_MASK | MODE_MASK | ENABLE_MASK;
}

void clockserver_init() {
    clock_init();
    RegisterAs("Clock Server");
}

void clockserver_task() {
    clockserver_init();
    Create(CLOCKNOTIFIER_PRIORITY, clocknotifier_task);

    CSmsg msg;
    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(CSmsg));
        if (sz != sizeof(CSmsg)) {
        } else {
            switch(msg.opcode) {
                // TODO: handle messages
                case UPDATE_TIME:
                case TIME_REQUEST:
                case DELAY_REQUEST:
                case DELAYUNTIL_REQUEST:
                default:
                    // TODO: throw error
                    break;
            }
        }
        // the error will be -2, if msg.err != 0
        Reply(sender, (void*) &msg, sizeof(CSmsg));
    }
}

// Clock notifier
void clocknotifier_task() {
    //clocknotifier_init();
    int server_tid = WhoIs("Clock Server");
    CSmsg msg;
    msg.type = UPDATE_TIME;
    for (;;) {
        // TODO: replace 0 with macro
        msg.data = AwaitEvent(0);
        Send(server_tid, &msg, sizeof(CSmsg));
    }
}
