#include <syscall.h>

int swi_jump(Request* request);

int Create(TASK_PRIORITY priority, void (*code)()) {
    Request request;
    request.opcode = CREATE;
    request.param[0] = priority;
    request.param[1] = (int)code;

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


