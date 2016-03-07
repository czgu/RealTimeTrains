#ifndef _TRAIN_LOGIC_TASK_H
#define _TRAIN_LOGIC_TASK_H

#define SWITCH_DIR_S 33
#define SWITCH_DIR_C 34
#define NUM_TRAIN_SWITCH 22

typedef enum {
    CMD_Q = 0,
    CMD_TR,
    CMD_RV,
    CMD_SW,
    CMD_WORKER_READY,
    CMD_RV_DONE,
    CMD_SENSOR_UPDATE,
    CMD_SENSOR_MODULE_AWAIT,
    CMD_CALIBRATE
} COMMAND_OP;

void train_command_server_task();
void train_command_worker_task();

void train_sensor_task();

// little tasks
void train_reverse_task();
void train_switch_task();

// send to server
void train_set_speed(int server_tid, int train, int speed);
void set_track(int server_tid, int track, int curve);


#endif
