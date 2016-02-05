#include <syscall.h>
#include <bwio.h>
#include <ts7200.h>

#include <clockserver.h>

#define TIMER_INIT_VAL 5084 //69

void clockserver_init() {
    unsigned int timer_base = TIMER3_BASE;
    volatile int* timer_loader = (int*)timer_base;
    volatile int* timer_control = (int*)(timer_base + CTRL_OFFSET);

    *timer_control = 0;
    *timer_control |= CLKSEL_MASK | MODE_MASK;
    *timer_loader = TIMER_INIT_VAL;

    *timer_control |= ENABLE_MASK;
}

void clockserver_task() {
    clockserver_init();
    
    for (;;) {
        int sender;
        CSmsg msg;
        int sz = Receive(&sender, &msg, sizeof(CSmsg));
        if (sz != sizeof(CSmsg)) {
        } else {
            switch(msg.opcode) {
            }
        }
        // the error will be -2, if msg.err != 0
        Reply(sender, (void*) &msg, sizeof(CSmsg));
    }
}
