#include <task.h>

typedef enum {
    CREATE = 0,
    TID,
    PID,
    PASS,
    EXIT,
    NONE
} SYSCALL;

typedef struct Request {
    SYSCALL opcode;
    unsigned int param[10];
} Request;

int Create(TASK_PRIORITY priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();
