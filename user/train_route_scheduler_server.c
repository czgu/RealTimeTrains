#include <syscall.h>

#include <terminal_mvc_server.h>
#include <train_route_scheduler_server.h>
#include <train_route_worker.h>

#include <train.h>
#include <string.h>

#include <assert.h>

static int line = 0;

void train_schedule_init(TrainSchedule* ts, int train_id) {
    ts->train_id = train_id;
    ts->loop = 0;
    
    ts->dest[0] = 0;
    ts->dest[1] = 0;

    ts->dest_len = 0;
    ts->curr_dest = 0;

    ts->status = 0;
}

void train_schedule_set_dest(TrainSchedule* ts, int* dest, int dest_len, int loop) {
    int i;

    ASSERT(dest_len <= 2);

    for (i = 0; i < dest_len; i++) {
        ts->dest[i] = dest[i];
    }

    ts->dest_len = dest_len;
    ts->curr_dest = 0;
    ts->loop = loop;
}


void train_route_scheduler_server() {
    RegisterAs("Route Scheduler");

    int i;
    int sender;
    TERMmsg request_msg;

    // reservation bitmap
    int number_of_trains = TRAIN_ID_MAX - TRAIN_ID_MIN + 1;

    TrainSchedule train_schedule[number_of_trains];
    for (i = 0; i < number_of_trains; i++) {
        train_schedule_init(train_schedule + i, i + TRAIN_ID_MIN);
    }

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            Reply(sender, 0, 0);
            switch(request_msg.opcode) {
                case MOVE_TRAIN: {
                    int train_id = request_msg.param[0];
                    int loop = request_msg.param[1];
                    int dest_len = MIN(request_msg.param[2], 2);
                    int dest[2];

                    for (i = 0; i < dest_len; i++)
                        dest[i] = request_msg.param[3 + i];
                           
                    TrainSchedule* ts = train_schedule + (train_id - TRAIN_ID_MIN);

                    train_schedule_set_dest(ts, dest, dest_len, loop);

                    pprintf(COM2, "\033[%d;%dH\033[K[%d] move train %d.", 25 + line ++ % 10, 1, Time(), train_id);

                    if (ts->status == 0) {
                        create_driver(ts);           
                    } else {
                        ts->status = 2;
                    }

                    break;
                }
                case LOOP_TRAIN:
                    break;
                case TRAIN_MOVED: {
                    int train_id = request_msg.param[0];
                    TrainSchedule* ts = train_schedule + (train_id - TRAIN_ID_MIN);

                    ts->status = 0;

                    if (ts->dest_len > 0) {
                        create_driver(ts);           
                    }
                    break;
                }
                case TRAIN_WAIT:
                    break;
            }
        }
    }
}

int create_driver(TrainSchedule* ts) {
    if (ts->dest_len <= 0) {
        return 1;
    }
    pprintf(COM2, "\033[%d;%dH\033[K[%d] create_driver.", 25 + line ++ % 10, 1, Time());

    // Create a train driver 
    int train_id = ts->train_id;
    int node = ts->dest[ts->curr_dest];

    char instruction[2] = {train_id, node};
    int cid = Create(10, train_route_worker);
    Send(cid, instruction, sizeof(char) * 2, 0, 0);

    // Update its pathing info
    ts->curr_dest ++;
    if (ts->curr_dest == ts->dest_len) { // we reached the end
        if (ts->loop) { // reset
            ts->curr_dest = 0;
        } else { // clear
            ts->dest_len = 0;
            ts->curr_dest = 0;
        }
    } 

    ts->status = 1;

    return 0;
}

void move_train(int scheduler_server, int train_id, int* nodes, int node_len, int loop) {
    TERMmsg msg;

    msg.opcode = MOVE_TRAIN;
    msg.param[0] = train_id;
    msg.param[1] = loop;
    msg.param[2] = node_len;

    int i;
    for (i = 0; i < node_len; i++)
        msg.param[3 + i] = nodes[i];

    Send(scheduler_server, &msg, sizeof(TERMmsg), 0, 0);
}

void driver_completed(int scheduler_server, int train_id, int status) {
    TERMmsg msg;
    msg.opcode = TRAIN_MOVED;
    msg.param[0] = train_id;
    msg.param[1] = status;

    Send(scheduler_server, &msg, sizeof(TERMmsg), 0, 0);
}
