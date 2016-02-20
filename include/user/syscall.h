#ifndef SYSCALL_H
#define SYSCALL_H

#include <task_type.h>

// K1
int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

// K2
extern int nameserver_tid;

int Send(int tid,void *message,int mslen,void *reply,int rplen);
int Receive(int *tid,void *msg,int msglen);
int Reply(int tid,void *reply,int replylen);

int RegisterAs(char *name);
int WhoIs(char *name);

// K3
extern int clockserver_tid;

int AwaitEvent(int eventid);
int Delay(int ticks);
int DelayUntil(int ticks);
int Time();

// K4
typedef enum {
    NOTIFIER_UPDATE = 0,
    PUTC,
    GETC,
    PUTLINE,
    GETLINE,
} IOOP;

typedef struct IOmsg {
    IOOP opcode;
    char str[100];
} IOmsg;

typedef struct GetRequest {
    int tid;
    IOOP opcode;
} GetRequest;

int Putc(int tid, int channel, char ch );
int Getc(int tid, int channel );

int PutStr(int tid, char* str, int len);


#endif
