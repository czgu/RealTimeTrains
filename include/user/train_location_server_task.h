#ifndef _TRAIN_LOCATION_SERVER_TASK_H_
#define _TRAIN_LOCATION_SERVER_TASK_H_

typedef enum {
    LOC_WAIT_SENSOR = 0,
    LOC_TRAIN_SPEED_UPDATE,
    LOC_SWITCH_UPDATE,
    LOC_SENSOR_MODULE_UPDATE,
    LOC_WHERE_IS
} LOCATION_OP;

void train_location_server_task();

#endif
