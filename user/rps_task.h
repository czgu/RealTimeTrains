#ifndef _RPS_TASK_H_
#define _RPS_TASK_H_

typedef struct RPS_Request {
    char opcode; // 'S', 'P', 'Q' 
    char param; // 'R' 'P' 'S'
    int ret;
} RPS_Request;

typedef struct RPS_Client {
    RPS_Request request;
    int tid;
} RPS_Client;

void rps_server();
void rps_client();

#endif
