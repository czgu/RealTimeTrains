#ifndef _TRAIN_LOGIC_TASK_H
#define _TRAIN_LOGIC_TASK_H

#define SWITCH_DIR_S 33
#define SWITCH_DIR_C 34
#define NUM_TRAIN_SWITCH 22

void train_command_task();
void train_listener_task();

// little tasks
void train_reverse_task();
void train_switch_task();

#endif
