#include <train_logic_task.h>
#include <terminal_mvc_server.h>
#include <train.h>
#include <syscall.h>
#include <rqueue.h>

#include <calibration.h>

#include <io.h>

// Server caches all information about the train
void train_command_server_task() {
    RegisterAs("Command Server");

    int i;
    TERMmsg request_msg;

    // Train Track initialization - Done in server
    // init train
    Train trains[81];
    for (i = 0; i < 81; i++) {
        train_init(trains + i, i);
    }

    // init switch
    for (i = 1; i <= NUM_TRAIN_SWITCH; i++) {
        train_cmd(SWITCH_DIR_S, i);
    }
    train_soloff();

    // init sensor
    unsigned short sensor_bitmap[SNLEN + 1] = {0};
    int sensor_await[SNLEN + 1] = {-1};

    // Create worker and its buffer
    Create(10, train_command_worker_task);
    
    TERMmsg command_buffer_pool[20];
    RQueue command_buffer;
    rq_init(&command_buffer, command_buffer_pool, 20, sizeof(TERMmsg));

    int command_worker_tid = -1;

    TERMmsg location_buffer_pool[20];
    RQueue courier_buffer;
    rq_init(&courier_buffer, location_buffer_pool, 20, sizeof(TERMmsg));
    

    int sender;
    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case CMD_Q:
                    Halt();
                    break;
                case CMD_TR:
                    Reply(sender, 0, 0);

                    // set speed in the model
                    train_setspeed(trains + (int)request_msg.param[0], request_msg.param[1]);

                    rq_push_back(&command_buffer, &request_msg);
                    break;
                case CMD_CALIBRATE:
                    Reply(sender, 0, 0);
                    rq_push_back(&command_buffer, &request_msg);
                    break;
                case CMD_RV:
                    Reply(sender, 0, 0);
                
                    // set the future reverse speed
                    request_msg.param[1] = trains[(int)request_msg.param[0]].speed;

                    // update the current speed to 0
                    train_setspeed(trains + (int)request_msg.param[0], 0);

                    rq_push_back(&command_buffer, &request_msg);
                    break;
                case CMD_SW:
                    Reply(sender, 0, 0);
                    rq_push_back(&command_buffer, &request_msg);
                    break;
                case CMD_WORKER_READY:
                    command_worker_tid = sender;
                    break;   
                case CMD_RV_DONE:
                    Reply(sender, 0, 0);
                    train_setspeed(trains + (int)request_msg.param[0], request_msg.param[1]);
                    break;
                case CMD_SENSOR_UPDATE: {
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

                    // FIXME: make it better
                    if (new_bitmap != 0 && sensor_await[module] >= 0) {
                        Reply(sensor_await[module], &new_bitmap, sizeof(unsigned short));
                        sensor_await[module] = -1;
                    }

                    break;
                }
                case CMD_SENSOR_MODULE_AWAIT: {
                    sensor_await[(int)request_msg.param[0]] = sender;
                    break;
                }
            }

            if (command_worker_tid >= 0) {
                if (command_worker_tid > 0 && !rq_empty(&command_buffer)) {
                    request_msg = *((TERMmsg *)rq_pop_front(&command_buffer));
                    Reply(command_worker_tid, &request_msg, sizeof(TERMmsg));
                    
                    command_worker_tid = -1;
                }
            }
        }
    }
}

// Worker does the work
void train_command_worker_task() {
    int command_server_tid = WhoIs("Command Server");
    char requestOP = CMD_WORKER_READY;

    TERMmsg command;

    for (;;) {
        int sz = Send(command_server_tid, &requestOP, sizeof(char), &command, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(command.opcode) {
                case CMD_CALIBRATE: {
                    int cid;

                    // TODO: switch
                    if (command.param[0] == 0) {
                        cid = Create(25, calibrate_stop);
                    } else if (command.param[0] == 1) {
                        cid = Create(25, calibrate_velocity);
                    } else if (command.param[0] == 2) {
                        cid = Create(25, calibrate_acceleration_move);
                    } else if (command.param[0] == 3) {
                        cid = Create(25, calibrate_acceleration_delta);
                    } else if (command.param[0] == 4) {
                        cid = Create(25, calibrate_stop_time);
                    }

                    Send(cid, &command, sizeof(TERMmsg), 0, 0);
        
                    break;
                }
                case CMD_TR:
                    train_cmd(command.param[1], command.param[0]);
                    break; 
                case CMD_RV: {
                    command.param[2] = command_server_tid;

                    int cid = Create(10, train_reverse_task);
                    Send(cid, &command, sizeof(TERMmsg), 0, 0);
                    break;
                }
                case CMD_SW: {
                    train_cmd(command.param[1], command.param[0]);
                    Create(10, train_switch_task);
                    break;
                }
            }
        }
    }
}

void train_reverse_task() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    if (command.opcode == CMD_RV) {
        train_cmd(0, command.param[0]);
        Delay(300);
        train_cmd(15, command.param[0]);
        train_cmd(command.param[1], command.param[0]);
    }

    int command_server_tid = WhoIs("Command Server");
    command.opcode = CMD_RV_DONE;
    // TODO: send to server
    Send(command_server_tid, &command, sizeof(TERMmsg), 0, 0);
}

void train_switch_task() {
    // turn off solenoid
    Delay(10);
    train_soloff();
}

void train_sensor_task() {
    int view_server = WhoIs("View Server");
    int command_server = WhoIs("Command Server");
    TERMmsg msg;

    SensorData sensors[SNLEN + 1];  // only sensors [1, SNLEN] are valid

    int i;
    for (;;) {
        sensor_request_upto(SNLEN);

        for (i = 1; i <= SNLEN; i++) {
            sensors[i].lo = Getc(COM1);
            sensors[i].hi = Getc(COM1);

            // send sensor data
            msg.opcode = CMD_SENSOR_UPDATE;
            msg.param[0] = i;
            msg.param[1] = sensors[i].lo;
            msg.param[2] = sensors[i].hi;

            Send(command_server, &msg, sizeof(TERMmsg), &msg, sizeof(TERMmsg));

            if (msg.param[1] != 0 || msg.param[2] != 0) {
                msg.opcode = DRAW_MODULE;
                Send(view_server, &msg, sizeof(TERMmsg), 0, 0);
            }
        }
    }
}

void train_set_speed(int server_tid, int train, int speed) {
    TERMmsg msg;
    msg.opcode = CMD_TR;
    msg.param[0] = train;
    msg.param[1] = speed;

    Send(server_tid, &msg, sizeof(TERMmsg), 0, 0);
}

void set_track(int server_tid, int track, int curve) {
    TERMmsg msg;
    msg.opcode = CMD_SW;
    msg.param[0] = track;
    msg.param[1] = curve ? SWITCH_DIR_C : SWITCH_DIR_S;

    Send(server_tid, &msg, sizeof(TERMmsg), 0, 0);
}

