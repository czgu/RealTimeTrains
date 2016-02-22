// std
#include <user_task.h>
#include <nameserver.h>
#include <clockserver.h>
#include <terminal_io_server.h>
#include <train_io_server.h>
#include <terminal_mvc_server.h>

#include <priority.h>

#include <train.h>
// user
//#include "rps_task.h"

// util
#include <io.h>
#include <bwio.h>
#include <syscall.h>
#include <string.h>

// temp
#include <train_sensor.h>

void empty_task() {
}

void idle_task() {
    int i = 0;
    for(;;) {
        i++;
    }
}

void basic_print_task() {
    int out_tid = WhoIs("UART2 Output");
    int in_tid = WhoIs("UART2 Input");

    DEBUG_MSG("basic_print_task\n\r");
    //Delay(100);
    Train train1, train2;
    train_init(&train1, 58);
    train_init(&train2, 63);

    //train_setspeed(&train2, 2);
    //train_reverse(&train1);
    train_setspeed(&train1, 10);
    Delay(500);

    train_setspeed(&train1, 0);
    //train_setspeed(&train1, 10);
    /*
    Delay(1000);
    train_setspeed(&train2, 2);*/
}

void first_task() {
    Create(IDLE_TASK_PRIORITY, idle_task);

    Create(NAMESERVER_PRIORITY, nameserver_task);
    Create(CLOCKSERVER_PRIORITY, clockserver_task);
 
   
    Create(TERMINAL_SERVER_PRIORITY, terminal_output_server_task);
    Create(TERMINAL_SERVER_PRIORITY, terminal_input_server_task);


    Create(TRAIN_SERVER_PRIORITY, train_output_server_task);
    Create(TRAIN_SERVER_PRIORITY, train_input_server_task);


    Create(10, terminal_controller_server_task);
    //Create(14, train_sensor_task);
    return;
}   
