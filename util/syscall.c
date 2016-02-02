#include <syscall.h>
#include <nameserver.h>
#include <string.h>

int swi_jump(Request* request);

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

    int ret = Send(NAMESERVER_TID, &msg, sizeof(msg), &msg, sizeof(msg));

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

    int ret = Send(NAMESERVER_TID, &msg, sizeof(msg), &msg, sizeof(msg));

    if (ret < 0) {
        return -1;
    }

    // -2: error happened during register
    return (msg.err < 0? -2: msg.binding.tid);
}
