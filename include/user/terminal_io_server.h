#ifndef _TERMINAL_IO_SERVER_H_
#define _TERMINAL_IO_SERVER_H_

#define TERM_GET_QUEUE_SIZE 10
#define TERM_GET_BUFFER_SIZE 100
#define TERM_COURIER_SIZE 40

#define TERM_PUT_BUFFER_SIZE 600

// Server task
void terminal_input_notifier_task();
void terminal_output_notifier_task();
void terminal_input_server_task();
void terminal_output_server_task();

void terminal_courier_task();

#endif
