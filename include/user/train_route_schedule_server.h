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

    short dest[10];
    short dest_len;
    short curr_dest;

    char status;
} TrainSchedule;

void train_schedule_init(TrainSchedule* ts);
void route_scheduler_server();



#endif
