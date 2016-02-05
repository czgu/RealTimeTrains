// std
#include <user_task.h>
#include <nameserver.h>
#include <clockserver.h>

#include <ts7200.h>

// user
#include "rps_task.h"

// util
#include <bwio.h>
#include <syscall.h>
#include <string.h>

#define MSG_SIZE 64

#define TIMER_INIT_VAL (0x1 << 20)

void test_time_receiver() {
    char msg[MSG_SIZE];

    int tid;
    Receive( &tid, msg, MSG_SIZE);
    Reply(tid, msg, MSG_SIZE);

}

void test_time_sender() {
    unsigned int timer_base = TIMER3_BASE;
    volatile int* timer_loader = (int*)timer_base;
    volatile int* timer_control = (int*)(timer_base + CTRL_OFFSET);

    *timer_control = 0;
    *timer_control |= CLKSEL_MASK | MODE_MASK;
    *timer_loader = TIMER_INIT_VAL;

    Create(0, test_time_receiver);

    char msg[MSG_SIZE];
    int i;
    for (i = 0; i < MSG_SIZE - 1; i++)
        msg[i] = 'a';
    msg[MSG_SIZE - 1] = 0;

    *timer_control |= ENABLE_MASK;

    int ret;
    ret = Send(65539, msg, MSG_SIZE, msg, MSG_SIZE);

    *timer_control &= ~ENABLE_MASK;

    int time_passed = TIMER_INIT_VAL - *((int*)(timer_base + VAL_OFFSET));

    int ms_passed = time_passed / 0.508469;

    bwprintf(COM2, "reply (%d): %s, time passed: %dus\n\r", ret, msg, ms_passed);

}

void first_task() {
    Create(1, nameserver_task);
    //Create(2, clockserver_task);

    //Create(HIGH, test_time_sender);

    Create(10, rps_server);

    for(;;) {
        //*((unsigned int*)VIC1_BASE +VIC_SOFTINT) = 1;
        Pass();
    }

    
    return;

}   

