#ifndef _TRAIN_IO_SERVER_H_
#define _TRAIN_IO_SERVER_H_

#define TRAIN_GET_QUEUE_SIZE 10
#define TRAIN_GET_BUFFER_SIZE 32

#define TRAIN_PUT_BUFFER_SIZE 100

#include <syscall.h>

// Server task
void train_input_notifier_task();
void train_output_notifier_task();
void train_input_server_task();
void train_output_server_task();

#endif
