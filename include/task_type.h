#ifndef _TASK_TYPE_H_
#define _TASK_TYPE_H_

// TASK
#define TASK_NPRIORITIES 32

typedef enum {
    READY = 0,
    ACTIVE,
    ZOMBIE,
    SEND_BLOCKED,
    RECEIVE_BLOCKED,
    REPLY_BLOCKED
} TASK_STATE;

// SYSCALL
typedef enum {
    CREATE = 0,
    TID, // 1
    PID, // 2
    PASS, // 3
    EXIT, // 4
    SEND, // 5
    RECEIVE, // 6
    REPLY, // 7
    NONE
} SYSCALL;


typedef struct Request {
    SYSCALL opcode;
    unsigned int param[10];
} Request;


#endif
