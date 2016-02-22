#ifndef _TERMINAL_MVC_SERVER_H_
#define _TERMINAL_MVC_SERVER_H_

#define INPUT_BUFFER_LEN 80

typedef enum {
    TIME_UPDATE = 0,
    INPUT_UPDATE,
    SENSOR_UPDATE,
    KERNEL_STATS_UPDATE,
    VIEW_READY,
    TRAIN_CMD_READY
} CONTROLLER_OP;

typedef enum {
    DRAW_CHAR = 0,
    DRAW_TIME,
    DRAW_SENSOR,
    DRAW_STATS,
    DRAW_CMD
} VIEW_OP;

typedef enum {
    CMD_Q = 0,
    CMD_TR,
    CMD_RV,
    CMD_SW
} COMMAND_OP;

typedef struct TERMmsg {
    char opcode;
    char param[3];
} TERMmsg;



void terminal_controller_server_task();
void terminal_view_listener_task();
void terminal_input_listener_task();
void terminal_time_listener_task();

void terminal_kernel_status_listener_task();

//HELPERS
int parse_command_block(char* str, int str_len, TERMmsg* cmd);

#endif