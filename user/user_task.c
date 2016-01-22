#include <user_task.h>

#include <bwio.h>
#include <syscall.h>

void first_task() {
    register int r0 asm("r0");

    int i = 4;

    bwprintf(COM2, "~~~I'm a barbie girl with %d teddy bears <3\n\r", r0);

    Pass();

    int tid = MyTid();
    int pid = MyParentTid();

    bwprintf(COM2, "tid:%d, pid:%d~~~in the barbie world, life is %dtastic\n\r", tid, pid,i);
    
    Exit();

    bwprintf(COM2, "~~~you can touch my hair\n\r");

}   
