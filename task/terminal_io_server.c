#include <terminal_io_server.h>
#include <rqueue.h>
#include <syscall.h>
#include <priority.h>

//#include <bwio.h> //temp
#include <io.h> //temp

void terminal_input_notifier_task() {
    int server_tid = WhoIs("UART2 Input");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;
    for (;;) {
        msg.str[0] = AwaitEvent(COM2_RECEIVE_IRQ);
        int err = Send(server_tid, &msg, sizeof(IOOP) + sizeof(char), 0, 0);

        (void)err;
    }
}

void terminal_output_notifier_task() {
    int server_tid = WhoIs("UART2 Output");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;
    for (;;) {
        char out;

        int* write_loc = (int *)AwaitEvent(COM2_SEND_IRQ);
        int err = Send(server_tid, &msg, sizeof(IOOP), &out, sizeof(char));

        (void)err;

        *write_loc = out;
        
    }
}

inline char buffer_pop_char(RQueue* buffer, int* numLines) {
    char c = *((char *)rq_pop_front(buffer));
    if (c == '\r') {
        (*numLines)--;
    }
    return c;
}

inline int buffer_pop_line(RQueue* buffer, char* outstr, int len, int* numLines) {
    int i;
    for (i = 0; i < len - 1; i++) {
        outstr[i] = *((char *)rq_pop_front(buffer));
        if (outstr[i] == '\r') {
            (*numLines)--;
            outstr[i + 1] = '\0';
            break;
        }
    }
    return i + 2;
}

void terminal_input_server_task() {
    // initialization
    RegisterAs("UART2 Input");

    GetRequest get_tasks[TERM_GET_QUEUE_SIZE]; // used to cache task tid that calls getc
    RQueue get_queue;
    rq_init(&get_queue, get_tasks, TERM_GET_QUEUE_SIZE, sizeof(GetRequest));
 
    char get_buffer[TERM_GET_BUFFER_SIZE];
    RQueue buffer;
    rq_init(&buffer, get_buffer, TERM_GET_BUFFER_SIZE, sizeof(char));

    char c_buffer[TERM_COURIER_SIZE];
    RQueue courier_buffer;
    rq_init(&courier_buffer, c_buffer, TERM_COURIER_SIZE, sizeof(char));
    int courier_tid = -1;

    Create(TERMINAL_NOTIFIER_PRIORITY, terminal_input_notifier_task);

    IOmsg msg;
    int numLines = 0;

    char backspace[3] = {'\b', ' ', '\b'};

    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(IOmsg));
        if (sz >= sizeof(IOOP)) {
            switch(msg.opcode) {
            case NOTIFIER_UPDATE:
            {
                char c = msg.str[0];
                Reply(sender, 0, 0);
                
                if (c == '\b') {
                    if (!rq_empty(&buffer)) {
                        buffer_pop_char(&buffer, &numLines);
                    }

                    // laggy implementation
                    if (courier_tid > 0) {
                        Reply(courier_tid, backspace, sizeof(char) * 3);
                        courier_tid = -1;
                    } else {
                        rq_push_back(&courier_buffer, backspace);
                        rq_push_back(&courier_buffer, backspace + 1);
                        rq_push_back(&courier_buffer, backspace + 2);
                    }
                } else {
                    rq_push_back(&buffer, &c);
                    // TODO: implement backspace
                    if (c == '\r') {
                        numLines++;
                    }

                    if (courier_tid > 0) {
                        Reply(courier_tid, &c, sizeof(char));
                        courier_tid = -1;
                    } else {
                        rq_push_back(&courier_buffer, &c);
                    }
                }


                if (!rq_empty(&get_queue)) {
                    GetRequest* gr = (GetRequest*)rq_first(&get_queue);
                    if (gr->opcode == GETC) {
                        rq_pop_front(&get_queue);
                        if (c != '\b')
                            buffer_pop_char(&buffer, &numLines);
                        Reply(gr->tid, &c, sizeof(char));
                    } else if (gr->opcode == GETLINE && numLines > 0) {
                        rq_pop_front(&get_queue);
                        char str[TERM_GET_BUFFER_SIZE];
                        int len = buffer_pop_line(&buffer, str,
                                                  TERM_GET_BUFFER_SIZE,
                                                  &numLines);
                        Reply(gr->tid, str, len * sizeof(char));
                    }
                }
                break;
            }
            case COURIER:
            {
                if(!rq_empty(&courier_buffer)) {
                    char str[TERM_COURIER_SIZE];
                    int len = 0;
                    do {
                        str[len++] = *((char*)rq_pop_front(&courier_buffer));
                    } while (!rq_empty(&courier_buffer));
                    Reply(sender, str, len * sizeof(char));   
                } else {
                    courier_tid = sender;
                }     
            }
            case GETC:
                if (!rq_empty(&buffer)) {
                    char c = buffer_pop_char(&buffer, &numLines);
                    Reply(sender, &c, sizeof(char));
                } else {
                    GetRequest gr;
                    gr.tid = sender; 
                    gr.opcode = GETC;
                    rq_push_back(&get_queue, &gr);
                }
                break;
            case GETLINE:
                if (numLines > 0) {
                    char str[TERM_GET_BUFFER_SIZE];
                    int len = buffer_pop_line(&buffer, str, 
                                              TERM_GET_BUFFER_SIZE, 
                                              &numLines);
                    Reply(sender, str, len * sizeof(char));
                } else {
                    GetRequest gr;
                    gr.tid = sender; 
                    gr.opcode = GETLINE;
                    rq_push_back(&get_queue, &gr);
                }
                break;
            default:
                break;
            }
        }
    }

    
}

void terminal_output_server_task() {
    // initialization
    RegisterAs("UART2 Output");

    char put_buffer[TERM_PUT_BUFFER_SIZE];
    RQueue buffer;
    rq_init(&buffer, put_buffer, TERM_PUT_BUFFER_SIZE, sizeof(char));

    Create(TERMINAL_NOTIFIER_PRIORITY, terminal_output_notifier_task);

    IOmsg msg;

    int notifier_tid = -1;

    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(IOmsg));
        int msg_len = sz - sizeof(IOOP);

        if (msg_len >= 0) {
            switch(msg.opcode) {
            case NOTIFIER_UPDATE:
                if (!rq_empty(&buffer)) {
                    char out = *((char *)rq_pop_front(&buffer));
                    Reply(sender, &out, sizeof(char)); 
                } else {
                    notifier_tid = sender;
                }
                break;
            case PUTC:
                if (msg_len == 0)
                    break;

                char c = msg.str[0];
                if (notifier_tid > 0) {
                    Reply(notifier_tid, &c, sizeof(char));
                    notifier_tid = -1;
                } else {
                    rq_push_back(&buffer, &c);
                }

                Reply(sender, 0, 0);

                break;
            case PUTSTR:
            {
                if (msg_len == 0)
                    break;

                // now the first byte
                int c = msg.str[0];
                if (notifier_tid > 0) {
                    Reply(notifier_tid, &c, sizeof(char));
                    notifier_tid = -1;
                } else {
                    rq_push_back(&buffer, &c);
                }

                // other bytes
                int i = 1;
                while (i < msg_len) {
                    rq_push_back(&buffer, &msg.str[i++]);
                }

                Reply(sender, 0, 0);
                break;
            }
            default:
                break;
            }
        }
    }
}

void terminal_courier_task() {
    RegisterAs("UART2 Courier");

    int terminal_output_server_tid, terminal_input_server_tid;
    terminal_output_server_tid = WhoIs("UART2 Output");
    terminal_input_server_tid = WhoIs("UART2 Input");

    //while ((terminal_output_server_tid = WhoIs("UART2 Output")) < 0);
    //while ((terminal_input_server_tid = WhoIs("UART2 Input")) < 0);

    IOmsg msg;

    for (;;) {
        msg.opcode = COURIER;    
        int len = Send(terminal_input_server_tid, &msg, sizeof(IOOP), &msg.str, IOMSG_STRLEN);

        if (len > 0) {
            msg.opcode = PUTSTR;
            Send(terminal_output_server_tid, &msg, sizeof(IOOP) + sizeof(char) * len, 0, 0);
        }
    }
}

