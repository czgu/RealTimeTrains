#ifndef _TERMINAL_IO_SERVER_H_
#define _TERMINAL_IO_SERVER_H_

#define GET_QUEUE_SIZE 10
#define GET_BUFFER_SIZE 30

#define PUT_BUFFER_SIZE 255

// Server task
void terminal_input_notifier_task();
void terminal_output_notifier_task();
void terminal_input_server_task();
void terminal_output_server_task();

void terminal_courier_task();

#endif
