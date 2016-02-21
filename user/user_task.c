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

void basic_print_task() {
    int out_tid = WhoIs("UART2 Output");
    int in_tid = WhoIs("UART2 Input");

    Train train;
    train_init(&train, 58);
    train_setspeed(&train, 6);

    for (;;) {
        char c;
        c = Getc(1);
        int i = Time();
        //pprintf("you typed: %c %d\n\r", c, i);
    }
}

void first_task() {
    Create(IDLE_TASK_PRIORITY, idle_task);

    Create(NAMESERVER_PRIORITY, nameserver_task);
    Create(CLOCKSERVER_PRIORITY, clockserver_task);
 
   
    Create(TERMINAL_SERVER_PRIORITY, terminal_output_server_task);
    Create(TERMINAL_SERVER_PRIORITY, terminal_input_server_task);


    Create(TRAIN_SERVER_PRIORITY, train_output_server_task);
    Create(TRAIN_SERVER_PRIORITY, train_input_server_task);


    Create(15, terminal_controller_server_task);
    //Create(15, basic_print_task);
    return;
}   


