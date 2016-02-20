#include <syscall.h>

#include <clockserver.h>
#include <nameserver.h>

#include <bwio.h>
#include <string.h>

int swi_jump(Request* request);

// server tids
int nameserver_tid = 0;
// FIXME: clockserver_tid is not initialized to 0
int clockserver_tid = -1;

int uart_output_tid[2] = {-1, -1};
int uart_input_tid[2] = {-1, -1};

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

    int ret = Send(nameserver_tid, &msg, sizeof(msg), &msg, sizeof(msg));

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

    int ret = Send(nameserver_tid, &msg, sizeof(msg), &msg, sizeof(msg));

    if (ret < 0) {
        return -1;
    }

    // -2: error happened during register
    return (msg.err < 0? -2: msg.binding.tid);
}

int AwaitEvent(int eventid) {
    Request request;
    request.opcode = AWAITEVENT;
    request.param[0] = eventid;

    return swi_jump(&request);
}

inline int find_clockserver() {
    if (clockserver_tid < 0) {
        clockserver_tid = WhoIs("Clock Server");
    }
    
    return clockserver_tid;
}

int Delay(int ticks) {
    CSmsg msg;
    msg.opcode = DELAY_REQUEST;
    msg.data = ticks;

    int tid = find_clockserver();
    if (tid < 0) 
        return -2;

    if (Send(tid, &msg, sizeof(msg), 0, 0) < 0)
        return -1;
    return 0;
}

int DelayUntil(int ticks) {
    CSmsg msg;
    msg.opcode = DELAYUNTIL_REQUEST;
    msg.data = ticks;

    int tid = find_clockserver();
    if (tid < 0) 
        return -2;

    if (Send(tid, &msg, sizeof(msg), 0, 0) < 0)
        return -1;
    return 0;
}

int Time() {
    CSmsg msg;
    msg.opcode = TIME_REQUEST;

    int tid = find_clockserver();
    if (tid < 0) 
        return -2;

    if (Send(tid, &msg, sizeof(msg), &msg, sizeof(msg)) < 0)
        return -1;
    
    return msg.data;
}

int get_io_server_tid(short in, short channel) {
    int* tid = 0;
    if (in) { // input
        tid = uart_input_tid + channel;

    } else { // ouput
        tid = uart_output_tid + channel;
    }

    if (*tid < 0) {
        char search_str[16] = "UART  ";
        if (in) 
            strcat(search_str, "Input");
        else
            strcat(search_str, "Output");

        search_str[4] = '1' + channel;

        *tid = WhoIs(search_str);
    }

    return *tid;
}

int Putc(int channel, char ch ) {
    IOmsg msg;
    msg.opcode = PUTC;
    msg.str[0] = ch;

    if (Send(get_io_server_tid(0, channel), &msg, sizeof(IOOP) + sizeof(char), 0, 0) < 0)
        return -1;
    
    return 0;
}

int PutStr(int channel, char* str) {
    return PutnStr(channel, str, strlen(str));
}

int PutnStr(int channel, char* str, int len) {
    IOmsg msg;
    msg.opcode = PUTSTR;
    
    int i;
    for (i = 0; i < len; i++) {
        msg.str[i] = str[i];
    }

     if (Send(get_io_server_tid(0, channel), &msg, sizeof(IOOP) + sizeof(char) * len, 0, 0) < 0)
        return -1;
    
    return 0;
}

int Getc(int channel ) {
    IOmsg msg;
    msg.opcode = GETC;        

    char ret;

    if (Send(get_io_server_tid(1, channel), &msg, sizeof(IOOP), &ret, sizeof(char)) < 0)
        return -1;
    
    return ret;
}

int GetLine(int tid, char* str, int len) {
    IOmsg msg;
    msg.opcode = GETLINE;

    return Send(tid, &msg, sizeof(IOOP), str, sizeof(char) * len);
}
