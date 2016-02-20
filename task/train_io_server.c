#include <train_io_server.h>
#include <rqueue.h>
#include <syscall.h>
#include <priority.h>

void train_input_notifier_task() {
    int server_tid = WhoIs("UART1 Input");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;
    for (;;) {
        msg.c = AwaitEvent(COM1_RECEIVE_IRQ);
        int err = Send(server_tid, &msg, sizeof(IOmsg), 0, 0);

        (void)err;
    }
}

void train_output_notifier_task() {
    int server_tid = WhoIs("UART1 Output");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;
    for (;;) {
        int out;

        int* write_loc = (int *)AwaitEvent(COM1_SEND_IRQ);
        int err = Send(server_tid, &msg, sizeof(IOmsg), &out, sizeof(int));

        (void)err;

        *write_loc = out;
        
    }
}

void train_input_server_task() {
    // initialization
    RegisterAs("UART1 Input");

    GetRequest get_tasks[GET_QUEUE_SIZE]; // used to cache task tid that calls getc
    RQueue request_queue;       // outstanding read requests
    rq_init(&request_queue, get_tasks, TRAIN_GET_QUEUE_SIZE, sizeof(GetRequest));
 
    char get_buffer[GET_BUFFER_SIZE];
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

                rq_push_back(&buffer, &c);

                // check type of request: char or short
                if (!rq_empty(&request_queue)) {
                    GetRequest* gr = (GetRequest*) rq_first(&request_queue);
                    switch (gr->opcode) {
                        case GETC: {
                            rq_pop_front(&request_queue);
                            char c = *((char *)rq_pop_front(&buffer);
                            // FIXME: reply message is a char??
                            Reply(gr->tid, &c, sizeof(char));
                            break;
                        }
                        case GETLINE:
                            if (buffer.size >= 2) {
                                rq_pop_front(&request_queue);
                                char c1 = *((char *)rq_pop_front(&buffer);
                                char c2 = *((char *)rq_pop_front(&buffer);
                                
                                short s = (c1 << 8) | c2;
                                Reply(gr->tid, &s, sizeof(short));
                            }
                            break;
                        default:
                            break;
                    }
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
            case GETLINE:
                if (buffer.size >= 2) {
                    char c1 = *((char *)rq_pop_front(&buffer));
                    char c2 = *((char *)rq_pop_front(&buffer));
                    short s = (c1 << 8) | c2;
                    // FIXME: reply message is a short??
                    Reply(sender, &s, sizeof(short));
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

void train_output_server_task() {
    // initialization
    RegisterAs("UART1 Output");

    char put_buffer[PUT_BUFFER_SIZE];
    RQueue buffer;
    rq_init(&buffer, put_buffer, PUT_BUFFER_SIZE, sizeof(char));

    Create(TRAIN_NOTIFIER_PRIORITY, train_output_notifier_task);

    IOmsg msg;

    int notifier_tid = -1;

    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(IOmsg));
        if (sz == sizeof(IOmsg)) {
            switch(msg.opcode) {
            case NOTIFIER_UPDATE:
                if (!rq_empty(&buffer)) {
                    int out = *((int *)rq_pop_front(&buffer));
                    Reply(sender, &out, sizeof(int)); 
                } else {
                    notifier_tid = sender;
                }
                break;
            case PUTC:
            {
                int c = msg.c;
                if (notifier_tid > 0) {
                    Reply(notifier_tid, &c, sizeof(int));
                    notifier_tid = -1;
                } else {
                    rq_push_back(&buffer, &c);
                }
                Reply(sender, 0, 0);
                break;
            }
            case PUTLINE:
            {

            }
            default:
                break;
            }
        }
    }


}



