#ifndef _TRAIN_LOCATION_SERVER_TASK_H_
#define _TRAIN_LOCATION_SERVER_TASK_H_

#include <train.h>

typedef enum {
    LOC_WAIT_SENSOR = 0,
    LOC_TRAIN_SPEED_UPDATE,
    LOC_TRAIN_SPEED_REVERSE_UPDATE,
    LOC_SWITCH_UPDATE,
    LOC_SENSOR_MODULE_UPDATE,
    LOC_WHERE_IS,
    LOC_TIMER_UPDATE,
    LOC_SET_TRAIN_DEST,
    LOC_QUERY
} LOCATION_OP;

typedef struct WaitModule {
    int sensor_tid[16];
    int wait_num;
} WaitModule;

void wait_module_init(WaitModule* wm);
inline void wait_module_add(WaitModule* wm, char sensor, int tid);
inline void wait_module_update(WaitModule* wm, unsigned short bitmap);

void train_location_server_secretary_task();
void train_location_server_task();
void train_sensor_task();
void train_location_ticker();
void train_timed_stop_task();

void train_tracer_task();

void wait_sensor(int location_server_tid, char module, int sensor);
int where_is(int location_server_tid, int train_id, TrainModelPosition* position);
void stop_train_at(int location_server_tid, int train_id, int node_id, int dist);
int location_query(int location_server_tid, int query_type, int query_val);


#endif
