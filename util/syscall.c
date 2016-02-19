#include <syscall.h>

#include <clockserver.h>
#include <nameserver.h>

#include <bwio.h>
#include <string.h>

int swi_jump(Request* request);

// server tids
int nameserver_tid = 0;
// FIXME: clockserver_tid is not initialized to 0
int clockserver_tid = 0;

int Create(int priority, void (*code)()) {
    Request request;
    request.opcode = CREATE;
    request.param[0] = priority;
    request.param[1] = (unsigned int)code;

    return swi_jump(&request);
}

int MyTid() {
    Request request;
    request.opcode = TID;

    return swi_jump(&request);
}

int MyParentTid() {
    Request request;
    request.opcode = PID;

    return swi_jump(&request);
}

void Pass() {
    Request request;
    request.opcode = PASS;

    swi_jump(&request);
}

void Exit() {
    Request request;
    request.opcode = EXIT;

    swi_jump(&request);
}

int Send(int tid, void *message, int mslen, void *reply, int rplen) {
    Request request;
    request.opcode = SEND;
    request.param[0] = tid;
    request.param[1] = (unsigned int)message;
    request.param[2] = mslen;
    request.param[3] = (unsigned int)reply;
    request.param[4] = rplen;

    return swi_jump(&request);
}

int Receive(int *tid, void *msg, int msglen) {
    Request request;
    request.opcode = RECEIVE;
    request.param[0] = (unsigned int)tid;
    request.param[1] = (unsigned int)msg;
    request.param[2] = msglen;

    return swi_jump(&request);
}

int Reply(int tid, void *reply, int replylen ) {
    Request request;
    request.opcode = REPLY;
    request.param[0] = tid;
    request.param[1] = (unsigned int)reply;
    request.param[2] = replylen;

    return swi_jump(&request);
}

int RegisterAs(char *name) {
    NSmsg msg;
    msg.opcode = REGISTERAS;

    strcpy(msg.binding.name, name);

    int ret = Send(nameserver_tid, &msg, sizeof(msg), &msg, sizeof(msg));

    if (ret < 0) {
        return -1;    
    }

    // -2: error happened during register
    return (msg.err < 0? -2: 0);
}

int WhoIs(char *name) {
    NSmsg msg;
    msg.opcode = WHOIS;

    strcpy(msg.binding.name, name);

    int ret = Send(nameserver_tid, &msg, sizeof(msg), &msg, sizeof(msg));

    if (ret < 0) {
        return -1;
    }

    // -2: error happened during register
    return (msg.err < 0? -2: msg.binding.tid);
}

int AwaitEvent(int eventid) {
    Request request;
    request.opcode = AWAITEVENT;
    request.param[0] = eventid;

    return swi_jump(&request);
}

int find_clockserver() {
    int tid = WhoIs("Clock Server");
    if (tid < 0)
        return -1;
    
    return tid;
}

int Delay(int ticks) {
    CSmsg msg;
    msg.opcode = DELAY_REQUEST;
    msg.data = ticks;

    int tid = find_clockserver();
    if (tid < 0) 
        return -2;

    if (Send(tid, &msg, sizeof(msg), 0, 0) < 0)
        return -1;
    return 0;
}

int DelayUntil(int ticks) {
    CSmsg msg;
    msg.opcode = DELAYUNTIL_REQUEST;
    msg.data = ticks;

    int tid = find_clockserver();
    if (tid < 0) 
        return -2;

    if (Send(tid, &msg, sizeof(msg), 0, 0) < 0)
        return -1;
    return 0;
}

int Time() {
    CSmsg msg;
    msg.opcode = TIME_REQUEST;

    int tid = find_clockserver();
    if (tid < 0) 
        return -2;

    if (Send(tid, &msg, sizeof(msg), &msg, sizeof(msg)) < 0)
        return -1;
    
    return msg.data;
}

int Putc(int tid, int channel, char ch ) {
    IOmsg msg;
    msg.opcode = PUTC;

    if (Send(tid, &msg, sizeof(IOmsg), 0, 0) < 0)
        return -1;
    
    return 0;
}

int Getc(int tid, int channel ) {
    IOmsg msg;
    msg.opcode = GETC;        

    int ret;

    if (Send(tid, &msg, sizeof(IOmsg), &ret, sizeof(int)) < 0)
        return -1;
    
    return ret;
}

