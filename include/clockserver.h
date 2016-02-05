#ifndef _CLOCKSERVER_H_
#define _CLOCKSERVER_H_

// Clock server
typedef enum {
    UPDATE_TIME = 0,
    TIME_REQUEST,
    DELAY_REQUEST,
    DELAYUNTIL_REQUEST,
} CSOP;

typedef struct CSmsg {
    CSOP opcode;
    //int type;
    int data;
    int err;
} CSmsg;

void clockserver_init();
void clockserver_task();

// Clock notifier
void clocknotifier_task();

#endif
