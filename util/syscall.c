#include <syscall.h>

int swi_jump(SYSCALL code);

int Create(int priority, void (*code)()) {
    return swi_jump(CREATE);
}

int MyTid() {
    return swi_jump(TID);
}

int MyParentTid() {
    return swi_jump(PID);
}

void Pass() {
    swi_jump(PASS);
}

void Exit() {
    swi_jump(EXIT);
}


