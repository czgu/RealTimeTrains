#include <terminal_mvc_server.h>

#include <terminal_gui.h>
#include <train_logic_task.h>

#include <syscall.h>
#include <io.h>

#include <string.h>
#include <bqueue.h>
#include <rqueue.h>

void terminal_controller_server_task() {
    RegisterAs("term controller");

    Create(10, terminal_input_listener_task);
    Create(10, terminal_time_listener_task);
    Create(10, terminal_view_listener_task);
    
    Create(10, train_command_task);

    char input_buffer[INPUT_BUFFER_LEN + 1];
    int input_len = 0;

    TERMmsg draw_buffer_pool[20];
    RQueue draw_buffer;
    rq_init(&draw_buffer, draw_buffer_pool, 20, sizeof(TERMmsg));

    TERMmsg command_buffer_pool[20];
    RQueue command_buffer;
    rq_init(&command_buffer, command_buffer_pool, 20, sizeof(TERMmsg));

    TERMmsg controller_msg;
    TERMmsg draw_msg;    

    int view_listener_tid = -1;
    int train_command_tid = -1;

    int sender;
    for (;;) {
        int size = Receive(&sender, &controller_msg, sizeof(TERMmsg));
        if (size >= sizeof(char)) {
            switch(controller_msg.opcode) {
            case TIME_UPDATE:
                Reply(sender, 0, 0);                

                draw_msg.opcode = DRAW_TIME;
                rq_push_back(&draw_buffer, &draw_msg);

                break;
            case INPUT_UPDATE: {
                char c = controller_msg.param[0];
                Reply(sender, 0, 0);                

                draw_msg.opcode = DRAW_CHAR;
                draw_msg.param[0] = c;
                rq_push_back(&draw_buffer, &draw_msg);

                if (c == '\r') {
                    int parse_succ = parse_command_block(input_buffer, input_len, &controller_msg);
                    if (parse_succ == 0) {
                        // push some output
                        draw_msg.opcode = DRAW_CMD;
                        draw_msg.param[0] = controller_msg.opcode;
                        draw_msg.param[1] = controller_msg.param[0];
                        draw_msg.param[2] = controller_msg.param[1];
                        
                        rq_push_back(&draw_buffer, &draw_msg);

                        if (controller_msg.opcode == CMD_Q)
                            Halt();
                    
                        // push the command
                        rq_push_back(&command_buffer, &controller_msg);
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
                    // too much input, dont push
                    rq_pop_back(&draw_buffer);
                }
                break;
            }
            case SENSOR_UPDATE:
                break;
            case VIEW_READY:
                view_listener_tid = sender;
                break;
            case TRAIN_CMD_READY:
                train_command_tid = sender;
                break;
            }

            if (view_listener_tid > 0 && !rq_empty(&draw_buffer)) {   
                draw_msg = *((TERMmsg *)rq_pop_front(&draw_buffer));
                Reply(view_listener_tid, &draw_msg, sizeof(TERMmsg));
            }

            if (train_command_tid > 0 && !rq_empty(&command_buffer)) {
                controller_msg = *((TERMmsg *)rq_pop_front(&command_buffer));
                Reply(train_command_tid, &controller_msg, sizeof(TERMmsg));
            }
        }
    }
}

void terminal_view_listener_task() {
    char view_ready = VIEW_READY;
    TERMmsg draw_msg;

    Cursor cs;
    init_screen(&cs);


    int logical_server_tid = WhoIs("term controller");

    for (;;) {
        int sz = Send(logical_server_tid, &view_ready, sizeof(char), &draw_msg, sizeof(TERMmsg));
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
            }    
        }
    }
}

void terminal_input_listener_task() {
    int logical_server_tid = WhoIs("term controller");

    TERMmsg msg;
    msg.opcode = INPUT_UPDATE;

    for (;;) {
        msg.param[0] = Getc(COM2);
        Send(logical_server_tid, &msg, sizeof(char) * 2, 0, 0);
    }   
}

void terminal_time_listener_task() {
    char opcode = TIME_UPDATE;
    int logical_server_tid = WhoIs("term controller");

    for (;;) {
        Delay(10);
        Send(logical_server_tid, &opcode, sizeof(char), 0, 0);        
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

