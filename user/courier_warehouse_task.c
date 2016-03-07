#include <syscall.h>
#include <courier_warehouse_task.h>

void courier_task() {
    int sender, receiver, size = 0;
    char buffer[COURIER_BUFFER_SIZE];

    char courier_request = COURIER_NOTIF;


    Receive(&sender, &receiver, sizeof(int));
    Reply(sender, 0, 0);

    for (;;) {
        size = Send(sender, &courier_request, sizeof(char), &buffer, sizeof(char) * COURIER_BUFFER_SIZE);
        Send(receiver, &buffer, size, 0, 0);
    }
}
