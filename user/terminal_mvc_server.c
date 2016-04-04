#include <terminal_mvc_server.h>

#include <terminal_gui.h>
#include <train_logic_task.h>

#include <syscall.h>
#include <io.h>
#include <assert.h>

#include <train.h>
#include <string.h>
#include <bqueue.h>
#include <rqueue.h>
#include <priority.h>

void terminal_view_server_task() {
    RegisterAs("View Server");

    Create(HI_WORKER_PRIORITY, terminal_time_listener_task);
    Create(LOW_WORKER_PRIORITY, terminal_kernel_status_listener_task);
    Create(HI_WORKER_PRIORITY, terminal_view_worker_task);
    
    TERMmsg draw_buffer_pool[20];
    RQueue draw_buffer;
    rq_init(&draw_buffer, draw_buffer_pool, 20, sizeof(TERMmsg));

    TERMmsg request_msg;

    int view_worker_tid = -1;

    int sender;

    for (;;) {
        int size = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (size >= sizeof(char)) {
            switch(request_msg.opcode) {
            case DRAW_TIME:
            case DRAW_CHAR:
            case DRAW_KERNEL_STATS: 
            case DRAW_TRACK:
            case DRAW_CMD:
            case DRAW_TRAIN_LOC:
            case DRAW_TRAIN_SPEED:
                Reply(sender, 0, 0);
                rq_push_back(&draw_buffer, &request_msg);
                break;
            case DRAW_MODULE: {
                Reply(sender, 0, 0);
                char module = request_msg.param[0];
                int data = request_msg.param[1] << 8 | request_msg.param[2];

                int i;

                request_msg.opcode = DRAW_SENSOR;
                request_msg.param[0] = module;
                for (i = 0; i < 16; i++) {
                    if ((0x8000 >> i) & data) {
                        request_msg.param[1] = i + 1;
                        rq_push_back(&draw_buffer, &request_msg);
                    }
                }
                break;
            }
            case VIEW_WORKER_READY:
                view_worker_tid = sender;
                break;
            }

            if (view_worker_tid > 0 && !rq_empty(&draw_buffer)) {   
                request_msg = *((TERMmsg *)rq_pop_front(&draw_buffer));
                Reply(view_worker_tid, &request_msg, sizeof(TERMmsg));

                view_worker_tid = -1;
            }
        }
    }
}

void terminal_view_worker_task() {
    char view_ready = VIEW_WORKER_READY;
    TERMmsg draw_msg;

    Cursor cs;
    init_screen(&cs);

    // recently triggered sensors to display
    SensorId triggered_sensor_pool[30];
    RQueue triggered_sensors;
    rq_init(&triggered_sensors, 
            triggered_sensor_pool, 30, sizeof(SensorId));
    
    // track which trains are displayed in which rows on the terminal
    // the max number of rows is MAX_DISPLAY_TRAINS
    // each train prints its stats to a separate row
    int train_display_next_available_index = 0;
    // maps to row index in [0, MAX_DISPLAY_TRAINS)
    char train_display_mapping[TRAIN_ID_MAX - TRAIN_ID_MIN + 1];
    int i;
    for(i = 0; i < TRAIN_ID_MAX - TRAIN_ID_MIN + 1; i++) {
        // initialize to invalid row
        train_display_mapping[i] = MAX_DISPLAY_TRAINS;
    }

    int view_server_tid = WhoIs("View Server");

    for (;;) {
        int sz = Send(view_server_tid, &view_ready, sizeof(char), &draw_msg, sizeof(TERMmsg));
        if (sz > 0) {
            switch(draw_msg.opcode) {
                case DRAW_CHAR: {
                    char c = draw_msg.param[0];

                    if (c == '\b') {
                        if (cs.col > CSINPUTX) {
                            cs.col --;
                            print_backsp(&cs);
                        } else {
                            reset_cursor(&cs);
                        }
                    } else if(c == '\r') {
                        reset_cursor(&cs);
                    } else {
                        Putc(COM2, c);
                        cs.col++;
                    }
                
                    break;
                }
                case DRAW_TIME: {
                    print_time(&cs, Time());
                    break;
                }
                case DRAW_CMD:  {
                    // DEBUG ------------------------
                    pprintf(COM2, "\033[%d;%dH", 24, 1);
                    pprintf(COM2, "\033[K");

                    pprintf(COM2, "'%d %d %d'", draw_msg.param[0], draw_msg.param[1], draw_msg.param[2]);


                    if (draw_msg.param[0] == CMD_SW) {
                        print_switch(&cs, draw_msg.param[2], draw_msg.param[1]);
                    }
        

                    // print_msg(&cs, draw_msg.param);
                    break;
                }
                case DRAW_KERNEL_STATS: {
                    print_stats(&cs, draw_msg.extra);
                    break;
                }
                case DRAW_TRACK:
                    print_track(&cs, draw_msg.param[0]);
                    break;
                case DRAW_SENSOR: {
                    //print_msg(&cs, "draw sensor\n\r");
                    SensorId sensor;
                    sensor.module = draw_msg.param[0];
                    sensor.id = draw_msg.param[1];

                    if (triggered_sensors.size == MAX_RECENT_SENSORS) {
                        rq_pop_back(&triggered_sensors);
                    }
                    rq_push_front(&triggered_sensors, &sensor);
                    //print_sensor(&cs, index, sensor);
                    int i;
                    for (i = 0; i < triggered_sensors.size; i++) {
                        SensorId* sensor = (SensorId*)rq_get(&triggered_sensors, i);
                        print_sensor(&cs, i, *sensor);
                    }
                    break;
                }
                case DRAW_TRAIN_SPEED: {
                    int train = draw_msg.param[0];
                    int speed = draw_msg.param[1];
                    ASSERTP(TRAIN_ID_MIN <= train && train <= TRAIN_ID_MAX,
                            "train id %d out of range", train);

                    int row = train_display_mapping[train - TRAIN_ID_MIN];
                    if (row >= MAX_DISPLAY_TRAINS) {
                        // this train has not been assigned a row yet
                        row = (train_display_next_available_index++) % MAX_DISPLAY_TRAINS;
                        train_display_mapping[train - TRAIN_ID_MIN] = row;
                        clear_train_row(&cs, row);
                        // TODO: clear previous mapping
                    }
                    // print train id and speed
                    print_train_bulk(&cs, row, TRAIN_ID, TRAIN_SPEED, 
                                     "%d\t%d", train, speed);
                    break;
                }
                case DRAW_TRAIN_LOC: {
                    int train = draw_msg.param[0];
                    int src_node = draw_msg.param[1];
                    int dst_node = draw_msg.param[2];
                    int distance = (draw_msg.param[3] << 8) | draw_msg.param[4];
                    int next_sensor = draw_msg.param[5];
                    // TODO: print train id on speed update instead
                    ASSERTP(TRAIN_ID_MIN <= train && train <= TRAIN_ID_MAX,
                            "train id %d out of range", train);
                    int row = train_display_mapping[train - TRAIN_ID_MIN];
                    if (row >= MAX_DISPLAY_TRAINS) {
                        // this train has not been assigned a row yet
                        row = (train_display_next_available_index++) % MAX_DISPLAY_TRAINS;
                        train_display_mapping[train - TRAIN_ID_MIN] = row;
                        clear_train_row(&cs, row);
                        // TODO: clear previous mapping
                    }
                    print_train_bulk(&cs, row, TRAIN_ARC, TRAIN_NEXT_SENSOR,
                                     "(%s->%s)\t%d\t%s", 
                                     train_track[src_node].name,
                                     train_track[dst_node].name,
                                     distance,
                                     train_track[next_sensor].name);
                    break;
                }
            }
        }
    }
}

void terminal_input_listener_task() {
    int view_server_tid = WhoIs("View Server");
    int command_server_tid = WhoIs("Command Server");

    TERMmsg msg;
    msg.opcode = DRAW_CHAR;

    char input_buffer[INPUT_BUFFER_LEN + 1];
    int input_len = 0;

    for (;;) {
        char c = Getc(COM2);

        int send_to_draw = 1;

        // parse
        if (c == '\r') {
            TERMmsg command_msg;

            int parse_succ = parse_command_block(input_buffer, input_len, &command_msg);
            if (parse_succ == 0) {
                // push some output
                TERMmsg draw_msg;

                draw_msg.opcode = DRAW_CMD;
                draw_msg.param[0] = command_msg.opcode;
                draw_msg.param[1] = command_msg.param[0];
                draw_msg.param[2] = command_msg.param[1];
 
                Send(view_server_tid, &draw_msg, sizeof(TERMmsg), 0, 0);               

                //TODO: send to train server 
                Send(command_server_tid, &command_msg, sizeof(TERMmsg), 0, 0);
            } 

            input_len = 0;
        }
        else if (c == '\b') {
            if (input_len > 0)
                input_len --;
        }
        else if (input_len < INPUT_BUFFER_LEN) {
            input_buffer[input_len++] = c;
        } else {
            send_to_draw = 0; // don't draw when over flow
        }

        if (send_to_draw > 0) {
            // send to draw
            msg.param[0] = c;
            Send(view_server_tid, &msg, sizeof(char) * 2, 0, 0);
        }
    }   
}

void terminal_time_listener_task() {
    char opcode = DRAW_TIME;
    int view_server_tid = WhoIs("View Server");

    for (;;) {
        Delay(10);
        Send(view_server_tid, &opcode, sizeof(char), 0, 0);        
    }
}

void terminal_kernel_status_listener_task() {
    int view_server_tid = WhoIs("View Server");
    
    TERMmsg msg;
    msg.opcode = DRAW_KERNEL_STATS;

    for (;;) {
        msg.extra = AwaitEvent(KERNEL_STATS);

        Send(view_server_tid, &msg, sizeof(TERMmsg), 0, 0);
    }
}


// HELPERS
int parse_command_block(char* str, int str_len, TERMmsg* msg) {
    str[str_len] = 0;

    if (str_len > 25 || str_len == 0) {
        return -1;
    }
    
    if (str_len == 1 && strncmp(str, "q", 1) == 0) {
        msg->opcode = CMD_Q;
        return 0;
    } else if(str_len >= 3) {
        if (strncmp(str, "tr ", 3) == 0) {
            int speed, train;
            char* current_c = str + 3;
            a2i('0', &current_c, 10, &train);
            a2i('0', &current_c, 10, &speed);

            msg->opcode = CMD_TR;
            msg->param[0] = train;
            msg->param[1] = speed;
            return 0;

        } else if(strncmp(str, "rv ", 3) == 0) {
            int train;
            char* current_c = str + 3;
            a2i('0', &current_c, 10, &train);
    
            msg->opcode = CMD_RV;
            msg->param[0] = train;
            
            return 0;
        } else if(strncmp(str, "sw ", 3) == 0) {
            int switch_num, switch_dir;
            char* current_c = str + 3;
            a2i('0', &current_c, 10, &switch_num);

            if (*current_c == 'S') {
                switch_dir = STRAIGHT;
            } else if (*current_c == 'C') {
                switch_dir = CURVED;
            } else {
                return -1;
            }
            //a2i('0', &current_c, 10, &switch_dir);

            msg->opcode = CMD_SW;
            msg->param[0] = switch_num;
            msg->param[1] = switch_dir;
            return 0;
        } else if(strncmp(str, "cal ", 4) == 0) {
            int type, train, speed, module, sensor, param5;
            char* current_c = str + 4;
            a2i('0', &current_c, 10, &type);
            a2i('0', &current_c, 10, &train);
            a2i('0', &current_c, 10, &speed);
            a2i('0', &current_c, 10, &module);
            a2i('0', &current_c, 10, &sensor);
            a2i('0', &current_c, 10, &param5);

            msg->opcode = CMD_CALIBRATE;
            msg->param[0] = type;
            msg->param[1] = train;
            msg->param[2] = speed;
            msg->param[3] = module;
            msg->param[4] = sensor;
            msg->extra = param5;
            return 0;
        } else if(strncmp(str, "st ", 3) == 0) {
            int train, module, sensor, dist;

            char* current_c = str + 3;
            a2i('0', &current_c, 10, &train);
            a2i('0', &current_c, 10, &module);
            a2i('0', &current_c, 10, &sensor);
            a2i('0', &current_c, 10, &dist);

            msg->opcode = CMD_STOP_TRAIN;
            msg->param[0] = train;
            msg->param[1] = (module - 1) * 16 + sensor - 1;
            msg->param[2] = dist >> 8;
            msg->param[3] = dist & 0xFF;

            return 0;
        } else if(strncmp(str, "m", 1) == 0) {
            int mode, train, src, dest;

            switch(str[1]) {
                case 's':
                    mode = 0;
                    break;
                case 'm':
                    mode = 1;
                    break;
                case 'c':
                    mode = 2;
                    break;
                default:
                    return -1;
                    break;
            }

            char* current_c = str + 3;
            a2i('0', &current_c, 10, &train);

            if (mode == 0) {
                a2i('0', &current_c, 10, &dest);
            } else {
                a2i('0', &current_c, 10, &src);
                a2i('0', &current_c, 10, &dest);
            }
            
            msg->opcode = CMD_MOVE_TRAIN;
            msg->param[0] = mode;
            msg->param[1] = train;
            msg->param[2] = src;
            msg->param[3] = dest;

            return 0;
        }     
    } 
   
    return -1;    
}

