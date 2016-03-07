#include <terminal_mvc_server.h>
#include <train_location_server_task.h>

void train_location_server_task() {
    RegisterAs("Location Server");

    int sender;
    TERMmsg request_msg;

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case LOC_WAIT_SENSOR:
                    break;
                case LOC_TRAIN_SPEED_UPDATE:
                    break;
                case LOC_SWITCH_UPDATE:
                    break;
                case LOC_SENSOR_MODULE_UPDATE:
                    break;
                case LOC_WHERE_IS:
                    break;
            }
        }
    }

}
