// std
#include <user_task.h>
#include <nameserver.h>
#include <clockserver.h>
#include <terminal_io_server.h>
#include <train_io_server.h>

#include <terminal_mvc_server.h>
#include <train_logic_task.h>
#include <train_location_server_task.h>

#include <calibration.h>

#include <priority.h>

#include <train.h>
// user
//#include "rps_task.h"

// util
#include <io.h>
#include <bwio.h>
#include <syscall.h>
#include <string.h>

void empty_task() {
}

void idle_task() {
    int i = 0;
    for(;;) {
        i++;
    }
}

void bootstrap_user_task() {
    Create(TRAIN_LOCATION_SERVER_PRIORITY, train_location_server_task);


    Create(TERMINAL_VIEW_SERVER_PRIORITY, terminal_view_server_task);
    Create(TRAIN_COMMAND_SERVER_PRIORITY, train_command_server_task);

    Create(TERMINAL_INPUT_LISTENER_PRIORITY, terminal_input_listener_task);
    Create(TRAIN_SENSOR_TASK_PRIORITY, train_sensor_task);
}

void first_task() {
    Create(IDLE_TASK_PRIORITY, idle_task);

    Create(NAMESERVER_PRIORITY, nameserver_task);
    Create(CLOCKSERVER_PRIORITY, clockserver_task);
 
   
    Create(TERMINAL_SERVER_PRIORITY, terminal_output_server_task);
    Create(TERMINAL_SERVER_PRIORITY, terminal_input_server_task);


    Create(TRAIN_SERVER_PRIORITY, train_output_server_task);
    Create(TRAIN_SERVER_PRIORITY, train_input_server_task);

    // Boot strap user tasks
    bootstrap_user_task();

    return;
}
