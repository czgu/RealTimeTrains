// std
#include <user_task.h>
#include <nameserver.h>

// user
#include "rps_task.h"

// util
#include <bwio.h>
#include <syscall.h>
#include <string.h>

void test_registeras_task() {
    int tid;
    char msg[80];
    RegisterAs("server1");
    RegisterAs("server2");
    Receive( &tid, msg, 80);
}

void test_whois_task() {
    int id = WhoIs("server1");
    DEBUG_MSG("server1 tid is %d\n\r", id); 

    id = WhoIs("server2");

    DEBUG_MSG("server2 tid is %d\n\r", id); 
}

void test_name_server_task() {
    Create(10, test_registeras_task);
    Create(30, test_whois_task);
}

void test_send_receive_task() {
    char* msg = "send to task";
    char reply[80];
    
    int ret;

    //bwprintf(COM2, "Send: %s %d\n\r", msg, strlen(msg));
    ret = Send( 65536, msg, strlen(msg), reply, 80);
    
    bwprintf(COM2, "reply (status: %d): %s\n\r", ret, reply);
}

void test_send_receive_reply_task() {
    
    int j;
    for(j = 0; j < 2; j++) {

        Create(30, test_send_receive_task);
        Create(10, test_send_receive_task);
        Create(10, test_send_receive_task);
        Create(10, test_send_receive_task);

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
