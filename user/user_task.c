// std
#include <user_task.h>
#include <nameserver.h>
#include <clockserver.h>
#include <terminal_io_server.h>
#include <train_io_server.h>

#include <terminal_mvc_server.h>
#include <train_logic_task.h>
#include <train_location_server_task.h>
#include <train_route_reservation_server.h>
#include <train_route_scheduler_server.h>

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

#include <dijkstra.h>
#include <track_data.h>

void empty_task() {
}

void idle_task() {
    int i = 0;
    for(;;) {
        i++;
    }
}

void bootstrap_user_task() {
    Create(10, train_route_reservation_server);
    Create(10, train_route_scheduler_server);

    Create(TRAIN_LOCATION_SERVER_PRIORITY, train_location_server_task);


    Create(TERMINAL_VIEW_SERVER_PRIORITY, terminal_view_server_task);
    Create(TRAIN_COMMAND_SERVER_PRIORITY, train_command_server_task);

    Create(TERMINAL_INPUT_LISTENER_PRIORITY, terminal_input_listener_task);
    Create(TRAIN_SENSOR_TASK_PRIORITY, train_sensor_task);

}

void print_b() {
    // save cursor
    bwprintf(COM2, "\0337");
    
    // move cursor
    bwprintf(COM2, "\033[%d;%dHb start ", 10, 1);
    bwprintf(COM2, "\0337");
    bwprintf(COM2, "\0338");
    //print_a();

    bwprintf(COM2, "b end");

    // restore cursor
    bwprintf(COM2, "\0338");
}
void print_c() {
    //<ESC>[{start};{end}r
    // enable scrolling
    //bwprintf(COM2, "\033[10;30r");
    //bwprintf(COM2, "\033[?30h");
    //CSI ? Pm h  

    bwprintf(COM2, "\033[2J\033[H");
    bwprintf(COM2, "Train\tSpeed\tError");
    bwprintf(COM2, "\033[%d;%dH", 2, 1);
    bwprintf(COM2, "ab\t1234567\tab");
    bwprintf(COM2, "\033[%d;%dH", 3, 1);
    bwprintf(COM2, "cd\tcd\tcd");

    // try overwriting values in the table
    bwprintf(COM2, "\033[%d;%dH", 2, 1);    // first row
    bwprintf(COM2, "\033[I");              // tab forward 1
    bwprintf(COM2, "\033[8X");
    bwprintf(COM2, "EL");
    //bwprintf(COM2, "\033[3g");               // clear
    //bwprintf(COM2, "\033[0g");              // clear first row, second cell

    //bwprintf(COM2, "\033[%d;%dH", 10, 1);
    /*
    int i;
    for (i = 0; i < 50; i++) {
        bwprintf(COM2, "\033[%d;%dHhello world %d\n\r", 10, 1, i);
    }*/
    //print_b();
    //bwprintf(COM2, "c end ");
}

void first_task() {
    /*
    // reset terminal settings
    bwprintf(COM2, "\033c");
    print_c();
    //for(;;);
    //bwprintf(COM2, "\033c");
    bwprintf(COM2, "\033F");    // move cursor to lower left corner
    //for (;;);
    return;*/
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
