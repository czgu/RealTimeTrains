// std
#include <user_task.h>
#include <nameserver.h>
#include <clockserver.h>
#include <terminal_io_server.h>

#include <priority.h>

// user
//#include "rps_task.h"

// util
#include <io.h>
#include <syscall.h>
#include <string.h>

void idle_task() {
    int i = 0;
    for(;;) {
        i++;
    }
}

void basic_print_task() {    
    for (;;) {
        char c;
        c = Getc(1);
        int i = Time();
        pprintf("you typed: %c %d\n\r", c, i);

        //Putc(1, c);
    }
}

void first_task() {
    Create(NAMESERVER_PRIORITY, nameserver_task);
    Create(CLOCKSERVER_PRIORITY, clockserver_task);
    
    Create(TERMINAL_SERVER_PRIORITY, terminal_output_server_task);
    Create(TERMINAL_SERVER_PRIORITY, terminal_input_server_task);
    
    Create(IDLE_TASK_PRIORITY, idle_task);

    Create(15, basic_print_task);

    return;
}   


