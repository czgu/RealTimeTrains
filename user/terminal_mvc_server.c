#include <terminal_mvc_server.h>

#include <terminal_gui.h>
#include <train_logic_task.h>

#include <syscall.h>
#include <io.h>

#include <string.h>
#include <bqueue.h>
#include <rqueue.h>

#define MAX_RECENT_SENSORS 10

void terminal_view_server_task() {
    RegisterAs("View Server");

    Create(10, terminal_time_listener_task);
    Create(10, terminal_kernel_status_listener_task);
    Create(10, terminal_view_worker_task);
    
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
            case DRAW_CMD:
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

    SensorId triggered_sensor_pool[30];
    RQueue triggered_sensors;
    rq_init(&triggered_sensors, 
            triggered_sensor_pool, 30, sizeof(SensorId));

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
                    short val = (draw_msg.param[0] << 8) | draw_msg.param[1]; // hack
                    print_stats(&cs, val);
                    break;
                }   
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
        short stat = AwaitEvent(KERNEL_STATS);
        msg.param[0] = (stat & 0xFF00) >> 8; // upper 8
        msg.param[1] = (stat & 0xFF); // lower 8

        Send(view_server_tid, &msg, sizeof(TERMmsg), 0, 0);
    }
}


// HELPERS
int parse_command_block(char* str, int str_len, TERMmsg* msg) {
    str[str_len] = 0;

    if (str_len > 10 || str_len == 0) {
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
                switch_dir = SWITCH_DIR_S;
            } else if (*current_c == 'C') {
                switch_dir = SWITCH_DIR_C;
            } else {
                return -1;
            }
            //a2i('0', &current_c, 10, &switch_dir);

            msg->opcode = CMD_SW;
            msg->param[0] = switch_num;
            msg->param[1] = switch_dir;
            return 0;
        }
    } 
   
    return -1;    
}

