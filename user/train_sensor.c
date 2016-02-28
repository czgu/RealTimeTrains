#include <train_sensor.h>

#include <io.h>
#include <train.h>
#include <syscall.h>
#include <terminal_mvc_server.h>

void train_sensor_task() {
    int view_server = WhoIs("View Server");
    TERMmsg msg;
    msg.opcode = DRAW_MODULE;

    SensorData sensors[SNLEN + 1];  // only sensors [1, SNLEN] are valid

    int i;
    for (;;) {
        sensor_request_upto(SNLEN);

        for (i = 1; i <= SNLEN; i++) {
            sensors[i].lo = Getc(COM1);
            sensors[i].hi = Getc(COM1);

            // send sensor data
            if (sensors[i].lo != 0 || sensors[i].hi != 0) {
                msg.param[0] = i;
                msg.param[1] = sensors[i].lo;
                msg.param[2] = sensors[i].hi;

                Send(view_server, &msg, sizeof(TERMmsg), 0, 0);
                
            }

        }
    }
}
