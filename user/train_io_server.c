#include <train_io_server.h>
#include <rqueue.h>
#include <syscall.h>
#include <priority.h>

#include <io.h>

void train_input_notifier_task() {
    int server_tid = WhoIs("UART1 Input");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;
    for (;;) {
        msg.str[0] = AwaitEvent(COM1_RECEIVE_IRQ);
        int err = Send(server_tid, &msg, sizeof(IOOP) + sizeof(char), 0, 0);

        (void)err;
    }
}

void train_input_server_task() {
    // initialization
    RegisterAs("UART1 Input");

    GetRequest get_tasks[TRAIN_GET_QUEUE_SIZE]; // used to cache task tid that calls getc
    RQueue request_queue;       // outstanding read requests
    rq_init(&request_queue, get_tasks, TRAIN_GET_QUEUE_SIZE, sizeof(GetRequest));
 
    char get_buffer[TRAIN_GET_BUFFER_SIZE];
    RQueue buffer;              // buffered input
    rq_init(&buffer, get_buffer, TRAIN_GET_BUFFER_SIZE, sizeof(char));

    Create(TRAIN_NOTIFIER_PRIORITY, train_input_notifier_task);

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

                // check type of request: char or short
                if (!rq_empty(&request_queue)) {
                    GetRequest* gr = (GetRequest*) rq_first(&request_queue);
                    switch (gr->opcode) {
                    case GETC: {
                        rq_pop_front(&request_queue);
                        // FIXME: reply message is a char??
                        Reply(gr->tid, &c, sizeof(char));
                        break;
                    }
                    default:
                        break;
                    }
                } else {
                    rq_push_back(&buffer, &c);
                }
                break;
            }
            case GETC: 
                if (!rq_empty(&buffer)) {
                    char c = *((char *)rq_pop_front(&buffer));
                    // FIXME: reply message is a char??
                    Reply(sender, &c, sizeof(char));
                } else {
                    GetRequest gr;
                    gr.tid = sender;
                    gr.opcode = msg.opcode;
                    rq_push_back(&request_queue, &gr);
                }
                break;
            default:
                break;
            }
        }
    }
}

void train_output_notifier_task() {
    int server_tid = WhoIs("UART1 Output");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;

    // int line = 25;

    for (;;) {
        char out;

        int* write_loc = (int *)AwaitEvent(COM1_SEND_IRQ);
        
        Send(server_tid, &msg, sizeof(IOOP), &out, sizeof(char));

        //pprintf(COM2, "\033[%d;%dH", line++, 1);
        //PutStr(COM2, "\033[K");
        //pprintf(COM2, "write %d to %d", out, write_loc);

        //if (line > 50)
        //    line = 25;


        *write_loc = out;
    }
}

void train_modem_notifier_task() {
    int server_tid = WhoIs("UART1 Output");
    IOmsg msg;
    int cts_on = 0;
    msg.opcode = NOTIFIER2_UPDATE;
    for (;;) {
        cts_on = AwaitEvent(COM1_MODEM_IRQ);
        if (cts_on) {
            Send(server_tid, &msg, sizeof(IOOP), 0, 0);
        }
    }
}

void train_output_server_task() {
    // initialization
    RegisterAs("UART1 Output");

    char put_buffer[TRAIN_PUT_BUFFER_SIZE];
    RQueue buffer;
    rq_init(&buffer, put_buffer, TRAIN_PUT_BUFFER_SIZE, sizeof(char));

    Create(TRAIN_NOTIFIER_PRIORITY, train_output_notifier_task);
    Create(TRAIN_NOTIFIER_PRIORITY, train_modem_notifier_task);

    IOmsg msg;
    int cts_on = 1;             // CTS on
    int notifier_tid = -1;

    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(IOmsg));
        int msg_len = sz - sizeof(IOOP);

        if (msg_len >= 0) {
            switch(msg.opcode) {
                case NOTIFIER_UPDATE:
                    notifier_tid = sender;
                    break;
                case NOTIFIER2_UPDATE:
                    cts_on = 1;
                    Reply(sender, 0, 0);
                    break;
                case PUTC:
                case PUTSTR: {
                    int i;
                    for (i = 0; i < msg_len; i++) {
                        rq_push_back(&buffer, msg.str + i);
                    }
                    Reply(sender, 0, 0);
                    break;
                }
                default:
                    break;
            }

            if (cts_on > 0 && notifier_tid > 0 && !rq_empty(&buffer)) {
                char out = *((char *)rq_pop_front(&buffer));
                Reply(notifier_tid, &out, sizeof(char));

                notifier_tid = -1;
                cts_on = 0;
            }
        }
    }
}
