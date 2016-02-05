#ifndef _CLOCKSERVER_H_
#define _CLOCKSERVER_H_

typedef struct CSmsg {
    int opcode;
    int type;
    int param[2];
    int ret;
} CSmsg;

void clock_init();
void clockserver_task();


#endif
