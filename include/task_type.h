#ifndef _TASK_TYPE_H_
#define _TASK_TYPE_H_

// TASK
#define TASK_NPRIORITIES 4
typedef enum {
    LOW = 0,
    MED,
    HIGH,
    TOP,
} TASK_PRIORITY;

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
    TID,
    PID,
    PASS,
    EXIT,
    SEND,
    RECEIVE,
    REPLY,
    NONE
} SYSCALL;


typedef struct Request {
    SYSCALL opcode;
    unsigned int param[10];
} Request;


#endif
