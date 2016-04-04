#ifndef _TRAIN_ROUTE_SCHEDULER_SERVER_H_
#define _TRAIN_ROUTE_SCHEDULER_SERVER_H_

typedef enum {
    MOVE_TRAIN,
    LOOP_TRAIN,
    TRAIN_MOVED,
    TRAIN_WAIT
} TRAIN_SCHEDULE_OP;

//route schedule server
typedef struct TrainSchedule {
    short train_id;
    char loop;

    short dest[2];
    short dest_len;
    short curr_dest;

    /*
        0 - no driver
        1 - has driver
        2 - has driver/needs to stop
    */
    char status;
} TrainSchedule;

void train_schedule_init(TrainSchedule* ts, int train_id);
void train_schedule_set_dest(TrainSchedule* ts, int* dest, int dest_len, int loop);

void train_route_scheduler_server();
int create_driver(TrainSchedule* ts);

void move_train(int scheduler_server, int train_id, int* nodes, int node_len, int loop);
void driver_completed(int scheduler_server, int train_id, int status);

#endif
