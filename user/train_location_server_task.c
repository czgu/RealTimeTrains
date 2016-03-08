#include <terminal_mvc_server.h>
#include <train_logic_task.h>

#include <train_location_server_task.h>

#include <syscall.h>
#include <io.h>

#include <train.h>


void train_location_server_task() {
    RegisterAs("Location Server");

    // init train
    int i;
    Train trains[81];
    for (i = 0; i < 81; i++) {
        train_init(trains + i, i);
    }

    // init switch
    short switch_straight[NUM_TRAIN_SWITCH + 1] = {1};

    // init sensor
    unsigned short sensor_bitmap[SNLEN] = {0};

    WaitModule wait_modules[SNLEN];
    for (i = 0; i < SNLEN; i++)
        wait_module_init(wait_modules + i);


    // init track
    

    int sender;
    TERMmsg request_msg;

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case LOC_WAIT_SENSOR:
                    wait_module_add(wait_modules + request_msg.param[0], request_msg.param[1], sender);
                    break;
                case LOC_TRAIN_SPEED_UPDATE:
                    Reply(sender, 0, 0);

                    // set speed in the model
                    // train_setspeed(trains + (int)request_msg.param[0], request_msg.param[1]);
                    break;
                case LOC_TRAIN_SPEED_REVERSE_UPDATE:
                    Reply(sender, 0, 0);
                    //TODO: reverse the direction
                    // set speed in the model
                    // train_setspeed(trains + (int)request_msg.param[0], request_msg.param[1]);
                
                    break;
                case LOC_SWITCH_UPDATE:
                    Reply(sender, 0, 0);
                    switch_straight[(int)request_msg.param[0]] = request_msg.param[1] == SWITCH_DIR_S;

                    break;
                case LOC_SENSOR_MODULE_UPDATE: {
                    int module = request_msg.param[0];
                    unsigned short old_bitmap = sensor_bitmap[module];
                    unsigned short new_bitmap = request_msg.param[1] << 8 | request_msg.param[2];
                    unsigned short delta_bitmap = 0;

                    sensor_bitmap[module] = new_bitmap;

                    // bits 1: changed from 0 to 1, bits 0: otherwise
                    delta_bitmap = (new_bitmap ^ old_bitmap) & ~old_bitmap; 

                    request_msg.param[1] = delta_bitmap >> 8;
                    request_msg.param[2] = delta_bitmap & 0xFF;

                    Reply(sender, &request_msg, sizeof(TERMmsg));

                    wait_module_update(wait_modules + module, new_bitmap);

                    break;
                }
                case LOC_WHERE_IS:
                    break;
            }
            /*
            if (request_msg.opcode != LOC_SENSOR_MODULE_UPDATE) {
                pprintf(COM2, "\033[%d;%dH", 24 + i++ % 15, 1);
                pprintf(COM2, "location: %d,%d,%d", request_msg.opcode, request_msg.param[0], request_msg.param[1]);
            }
            */
        }
    }

}

void train_sensor_task() {
    int view_server = WhoIs("View Server");
    int location_server = WhoIs("Location Server");
    TERMmsg msg;

    SensorData sensors[SNLEN];  // only sensors [0, SNLEN - 1] are valid
                                // which is A = 0,B,C,D,E
    int i;
    for (;;) {
        sensor_request_upto(SNLEN);

        for (i = 0; i < SNLEN; i++) {
            sensors[i].lo = Getc(COM1);
            sensors[i].hi = Getc(COM1);

            // send sensor data
            msg.opcode = LOC_SENSOR_MODULE_UPDATE;
            msg.param[0] = i;
            msg.param[1] = sensors[i].lo;
            msg.param[2] = sensors[i].hi;

            Send(location_server, &msg, sizeof(TERMmsg), &msg, sizeof(TERMmsg));

            if (msg.param[1] != 0 || msg.param[2] != 0) {
                msg.opcode = DRAW_MODULE;
                Send(view_server, &msg, sizeof(TERMmsg), 0, 0);
            }
        }
    }
}

void wait_sensor(int location_server_tid, char module, int sensor) {
    TERMmsg msg;
    msg.opcode = LOC_WAIT_SENSOR;
    msg.param[0] = module - 1;
    msg.param[1] = sensor - 1;

    Send(location_server_tid, &msg, sizeof(TERMmsg), 0, 0);
}

void wait_module_init(WaitModule* wm) {
    int i;
    for (i = 0; i < 16; i++)
        wm->sensor_tid[i] = -1;
    wm->wait_num = 0;
}

inline void wait_module_add(WaitModule* wm, char sensor ,int tid) {
    if (wm->sensor_tid[(int)sensor] >= 0) {
        // TODO: do something to kick out the other waiting task?
    } else {
        wm->wait_num ++;
    }
    wm->sensor_tid[(int)sensor] = tid;
}

inline void wait_module_update(WaitModule* wm, unsigned short bitmap) {
    if (wm->wait_num == 0 || bitmap == 0)
        return;
    int i;
    for (i = 0; i < 16; i++) {
        if (wm->sensor_tid[i] >= 0 && (bitmap & (0x8000 >> i))) {
            Reply(wm->sensor_tid[i], 0, 0);
        }
    }

}

