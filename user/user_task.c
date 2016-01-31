// std
#include <user_task.h>
#include <nameserver.h>

// user
#include "rps_task.h"

// util
#include <bwio.h>
#include <syscall.h>
#include <string.h>

void a1_task() {
    int tid = MyTid();
    int pid = MyParentTid();

    bwprintf(COM2, "Tid: %d Parent Tid: %d\n\r", tid, pid);

    Pass();
    
    bwprintf(COM2, "Tid: %d Parent Tid: %d\n\r", tid, pid);

    Exit();
}

void a2_task() {
    char* msg = "send to task";
    char reply[80];
    
    int ret;

    //bwprintf(COM2, "Send: %s %d\n\r", msg, strlen(msg));
    ret = Send( 65536, msg, strlen(msg), reply, 80);
    
    bwprintf(COM2, "reply (status: %d): %s\n\r", ret, reply);
}

void a2_taska() {
    int tid;
    char msg[80];
    RegisterAs("coolest guy");
    RegisterAs("ugliest guy");
    int ret = Receive( &tid, msg, 80);
}

void a2_taskb() {
    int id = WhoIs("coolest guy");
    DEBUG_MSG("coolest guy is %d\n\r", id); 

    id = WhoIs("ugliest guy");

    DEBUG_MSG("ugliest guy is %d\n\r", id); 
}

void first_task() {
    Create(TOP, nameserver_task);

    Create(HIGH, rps_server);
    
    return;
    Create(HIGH, a2_taska);
    Create(LOW, a2_taskb);
    

    int j;
    for(j = 0; j < 2; j++) {

        Create(LOW, a2_task);
        Create(HIGH, a2_task);
        Create(HIGH, a2_task);
        Create(HIGH, a2_task);

        char msg[80];
        int tid;
        int ret;

        int i = 0;
        while (i < 4) {
            ret = Receive( &tid, msg, 80);

            char* reply = "reply to task  ";
            reply[14] = '0' + i;
            bwprintf(COM2, "received from %d (status %d): %s\n\r", tid, ret, msg);

            ret = Reply(tid, reply, strlen(reply));

            // bwprintf(COM2, "Sent reply (status %d)\n\r", ret);

            i++;
        }
    }
}   

