#include <train_logic_task.h>
#include <terminal_mvc_server.h>
#include <train_location_server_task.h>

#include <train_route_server.h>


#include <train.h>
#include <syscall.h>
#include <rqueue.h>

#include <courier_warehouse_task.h>
#include <calibration.h>

#include <io.h>
#include <priority.h>

#include <string.h>
#include <dijkstra.h>
#include <track_data.h>

// Server caches all information about the train
void train_command_server_task() {
    RegisterAs("Command Server");

    int i;
    TERMmsg request_msg;

    // Train Track initialization - Done in server
    short train_speed[81] = {0};

    // init switch to straight
    for (i = 1; i <= NUM_TRAIN_SWITCH; i++) {
        set_switch_normalized(i, STRAIGHT);
    }
    track_soloff();    

    // Create worker and its buffer
    Create(MED_WORKER_PRIORITY, train_command_worker_task);
    
    TERMmsg command_buffer_pool[20];
    RQueue command_buffer;
    rq_init(&command_buffer, command_buffer_pool, 20, sizeof(TERMmsg));

    int command_worker_tid = -1;

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
                    rq_push_back(&command_buffer, &request_msg);

                    train_speed[(int)request_msg.param[0]] = request_msg.param[1];
                    break;
                case CMD_CALIBRATE:
                case CMD_STOP_TRAIN:
                case CMD_CALCULATE_PATH:
                    Reply(sender, 0, 0);
                    rq_push_back(&command_buffer, &request_msg);
                    break;
                case CMD_RV:
                    Reply(sender, 0, 0);
                
                    // set the future reverse speed
                    request_msg.param[1] = train_speed[(int)request_msg.param[0]];
                    rq_push_back(&command_buffer, &request_msg);
                    break;
                case CMD_SW:
                    Reply(sender, 0, 0);
                    rq_push_back(&command_buffer, &request_msg);
                    break;
                case CMD_WORKER_READY:
                    command_worker_tid = sender;
                    break;   
            }

            if (command_worker_tid > 0 && !rq_empty(&command_buffer)) {
                request_msg = *((TERMmsg *)rq_pop_front(&command_buffer));
                Reply(command_worker_tid, &request_msg, sizeof(TERMmsg));
                
                command_worker_tid = -1;
            }
        }
    }
}

// Worker does the work
void train_command_worker_task() {
    int command_server_tid = WhoIs("Command Server");
    int location_server_tid = WhoIs("Location Server");
    char requestOP = CMD_WORKER_READY;

    TERMmsg command;

    char reserved_nodes[TRACK_MAX];
    memset(reserved_nodes, 0, TRACK_MAX*sizeof(char));

    for (;;) {
        int sz = Send(command_server_tid, &requestOP, sizeof(char), &command, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(command.opcode) {
                case CMD_CALIBRATE: {
                    int cid = 0;

                    // TODO: switch
                    if (command.param[0] == 0) {
                        cid = Create(MED_WORKER_PRIORITY, calibrate_stop);
                    } else if (command.param[0] == 1) {
                        cid = Create(MED_WORKER_PRIORITY, calibrate_velocity);
                    } else if (command.param[0] == 2) {
                        cid = Create(MED_WORKER_PRIORITY, calibrate_acceleration_move);
                    } else if (command.param[0] == 3) {
                        cid = Create(MED_WORKER_PRIORITY, calibrate_acceleration_delta);
                    } else if (command.param[0] == 4) {
                        cid = Create(MED_WORKER_PRIORITY, calibrate_stop_time);
                    } else if (command.param[0] == 5) {
                        // init track, don't do it unless on start up
                        if (command.param[1] == 0) {
                            init_tracka(train_track);
                        } else {
                            init_trackb(train_track);
                        }
                    } else if (command.param[0] == 6) {
                        cid = Create(MED_WORKER_PRIORITY, calibrate_find_train);
                    }

                    if (cid > 0)
                        Send(cid, &command, sizeof(TERMmsg), 0, 0);
        
                    break;
                }
                case CMD_CALCULATE_PATH: {
                    /*
                    Path path;

                    dijkstra_find(train_track + (unsigned int)command.param[0], train_track + (unsigned int)command.param[1], &path, reserved_nodes);

                    pprintf(COM2, "\033[%d;%dH\033[K", 35, 1);
                    int i;
                    for (i = path.path_len - 1; i >= 0; i--) {
                        pprintf(COM2, "%s (%d) %d -> ", path.nodes[i]->name, path.edges[i], reserved_nodes[path.nodes[i]->id]);
                    }
                    */

                    int cid = Create(10, train_route_worker);
                    Send(cid, command.param, sizeof(char) * 2, 0, 0);
                    break;
                }
                case CMD_STOP_TRAIN:
                    command.opcode = LOC_SET_TRAIN_DEST;
                    Send(location_server_tid, &command, sizeof(TERMmsg), 0, 0);
                    break;
                case CMD_TR:
                    train_set_speed(location_server_tid, command.param[0], command.param[1]);
                    break; 
                case CMD_RV: {
                    command.param[2] = command_server_tid;

                    int cid = Create(10, train_reverse_task);
                    Send(cid, &command, sizeof(TERMmsg), 0, 0);
                    break;
                }
                case CMD_SW: {
                    track_set_switch(location_server_tid, command.param[0], command.param[1], 1);
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

    int location_server_tid = WhoIs("Location Server");

    if (command.opcode == CMD_RV) {
        int train = command.param[0];

        train_set_speed(location_server_tid, train, 0);
        Delay(300);

        train_reverse(location_server_tid, train);
        train_set_speed(location_server_tid, train, command.param[1]);
    }
}

