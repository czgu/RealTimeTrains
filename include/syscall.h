typedef enum {
    CREATE = 0,
    TID,
    PID,
    PASS,
    EXIT
} SYSCALL;

int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();
