#include <terminal_io_server.h>
#include <rqueue.h>
#include <syscall.h>
#include <priority.h>

#include <bwio.h> //temp

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

void terminal_input_server_task() {
    // initialization
    RegisterAs("UART2 Input");

    int get_tasks[TERM_GET_QUEUE_SIZE]; // used to cache task tid that calls getc
    RQueue get_queue;
    rq_init(&get_queue, get_tasks, TERM_GET_QUEUE_SIZE, sizeof(int));
 
    char get_buffer[TERM_GET_BUFFER_SIZE];
    RQueue buffer;
    rq_init(&buffer, get_buffer, TERM_GET_BUFFER_SIZE, sizeof(char));

    Create(TERMINAL_NOTIFIER_PRIORITY, terminal_input_notifier_task);

    IOmsg msg;

    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(IOmsg));
        if (sz >= sizeof(IOOP)) {
            switch(msg.opcode) {
            case NOTIFIER_UPDATE:
            {
                char c = msg.str[0];
                Reply(sender, 0, 0);
                
                if (!rq_empty(&get_queue)) {
                    int tid = *((int *)rq_pop_front(&get_queue));
                    Reply(tid, &c, sizeof(char));
                } else {
                    rq_push_back(&buffer, &c);
                }
                    
                break;
            }
            case GETC:
                if (!rq_empty(&buffer)) {
                    char c = *((char *)rq_pop_front(&buffer));
                    Reply(sender, &c, sizeof(char));
                } else {
                    rq_push_back(&get_queue, &sender);
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
    while ((terminal_output_server_tid = WhoIs("UART2 Output")) < 0);
    while ((terminal_input_server_tid = WhoIs("UART2 Input")) < 0);

    

    for (;;) {
        //Send(terminal_input_server_tid,         
    }
}

