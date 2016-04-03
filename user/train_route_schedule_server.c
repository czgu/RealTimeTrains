#include <syscall.h>

#include <terminal_mvc_server.h>
#include <train_route_schedule_server.h>

#include <train.h>
#include <string.h>


void route_scheduler_server() {
    RegisterAs("Route Scheduler");

    int sender;
    TERMmsg request_msg;

    // reservation bitmap
    char train_status[TRAIN_ID_MAX - TRAIN_ID_MIN + 1];
    memset(train_status, 0, (TRAIN_ID_MAX - TRAIN_ID_MIN + 1) * sizeof(char));
    

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case MOVE_TRAIN:
                    break;
                case LOOP_TRAIN:
                    break;
                case TRAIN_MOVED:
                    break;
                case TRAIN_WAIT:
                    break;
            }
        }
    }
}


