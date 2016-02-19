#include <terminal_io_server.h>
#include <rqueue.h>
#include <syscall.h>
#include <priority.h>

void terminal_input_notifier_task() {
    int server_tid = WhoIs("UART2 Input");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;
    for (;;) {
        msg.c = AwaitEvent(COM2_RECEIVE_IRQ);
        int err = Send(server_tid, &msg, sizeof(IOmsg), 0, 0);

        (void)err;
    }
}

void terminal_output_notifier_task() {
    int server_tid = WhoIs("UART Output");
    IOmsg msg;
    msg.opcode = NOTIFIER_UPDATE;
    for (;;) {
        int out;

        int* write_loc = (int *)AwaitEvent(COM2_SEND_IRQ);
        int err = Send(server_tid, &msg, sizeof(IOmsg), &out, sizeof(int));

        (void)err;

        *write_loc = out;
        
    }
}

void terminal_input_server_task() {
    // initialization
    RegisterAs("UART2 Input");

    int get_tasks[GET_QUEUE_SIZE]; // used to cache task tid that calls getc
    RQueue get_queue;
    rq_init(&get_queue, get_tasks, GET_QUEUE_SIZE, sizeof(int));
 
    int get_buffer[GET_BUFFER_SIZE];
    RQueue buffer;
    rq_init(&buffer, get_buffer, GET_BUFFER_SIZE, sizeof(int));

    Create(TERMINAL_NOTIFIER_PRIOIRTY, terminal_input_notifier_task);

    IOmsg msg;

    for (;;) {
        int sender;
        int sz = Receive(&sender, &msg, sizeof(IOmsg));
        if (sz == sizeof(IOmsg)) {
            switch(msg.opcode) {
            case NOTIFIER_UPDATE:
            {
                int c = msg.c;
                Reply(sender, 0, 0);
                
                if (!rq_empty(&get_queue)) {
                    int tid = *((int *)rq_pop_front(&get_queue));
                    Reply(tid, &c, sizeof(int));
                } else {
                    rq_push_back(&buffer, &c);
                }
                    
                break;
            }
            case GETC:
                if (!rq_empty(&buffer)) {
                    int c = *((int *)rq_pop_front(&buffer));
                    Reply(sender, &c, sizeof(int));
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

    int put_buffer[PUT_BUFFER_SIZE];
    RQueue buffer;
    rq_init(&buffer, put_buffer, PUT_BUFFER_SIZE, sizeof(int));

    Create(TERMINAL_NOTIFIER_PRIOIRTY, terminal_output_notifier_task);

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
            default:
                break;
            }
        }
    }


}



