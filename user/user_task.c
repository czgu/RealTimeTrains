#include <user_task.h>

#include <bwio.h>
#include <syscall.h>

void a1_task() {
    int tid = MyTid();
    int pid = MyParentTid();

    bwprintf(COM2, "Tid: %d Parent Tid: %d\n\r", tid, pid);

    Pass();
    
    bwprintf(COM2, "Tid: %d Parent Tid: %d\n\r", tid, pid);

    Exit();
}

void first_task() {
    Create(LOW, a1_task);
    Create(LOW, a1_task);

    Create(HIGH, a1_task);
    Create(HIGH, a1_task);

    bwprintf(COM2, "First: exiting\n\r");
    Exit();
    
}   

