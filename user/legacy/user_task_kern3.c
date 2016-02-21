// std
#include <user_task.h>
#include <nameserver.h>
#include <clockserver.h>

#include <priority.h>

// user
#include "rps_task.h"

// util
#include <bwio.h>
#include <syscall.h>
#include <string.h>

void idle_task() {
    int i = 0;
    for(;;) {
        i++;
    }
}

void timer_task() {
    int tid = MyTid();
    int pid = MyParentTid();
    int time[2];

    Send(pid, 0, 0, time, sizeof(int) * 2);

    int i;
    for (i = 1; i <= time[1]; i++) {
        Delay(time[0]);
        bwprintf(COM2, "[%d] interval: %d iter: %d\n\r", tid, time[0], i);
    }
}

void first_task() {
    Create(NAMESERVER_PRIORITY, nameserver_task);
    Create(CLOCKSERVER_PRIORITY, clockserver_task);

    // Create(HIGH, test_time_sender);

    // Create(10, rps_server);

    Create(IDLE_TASK_PRIORITY, idle_task);

    Create(3, timer_task); // 65541
    Create(4, timer_task); // 65542
    Create(5, timer_task); // 65543
    Create(6, timer_task); // 65544

    int delaytimes[4] = {10, 23, 33, 71};
    int delaynum[4] = {20, 9, 6, 3};
    
    int i;
    int tid; int reply[2];
    for (i = 0; i < 4; i++) {
        Receive(&tid, 0, 0);
        reply[0] = delaytimes[tid - 65541];
        reply[1] = delaynum[tid - 65541];
        Reply(tid, reply, sizeof(int) * 2); 
    }

    return;
}   

