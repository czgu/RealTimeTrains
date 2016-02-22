#include <train_sensor.h>

#include <io.h>
#include <train.h>
#include <syscall.h>
#include <terminal_mvc_server.h>

void train_sensor_task() {
    int logical_term_server = WhoIs("term controller");
    TERMmsg msg;
    msg.opcode = SENSOR_UPDATE;

    SensorData sensors[SNLEN + 1];  // only sensors [1, SNLEN] are valid

    int i;
    for (i = 1; i <= SNLEN; i++) {
        sensor_reset(sensors + i);
        sensor_request(i);
    }

    for (i = 1;; i++) {
        sensors[i].lo = Getc(COM1);
        sensors[i].hi = Getc(COM1);

        // request sensor module again
        sensor_request(i);

        // send sensor data?
        if (sensors[i].lo != 0 || sensors[i].hi != 0) {
            msg.param[0] = i;               // sensor module
            msg.param[1] = sensors[i].lo;
            msg.param[2] = sensors[i].hi;

            Send(logical_term_server, &msg, sizeof(TERMmsg), 0, 0);
        }
        if (i == SNLEN) {
            // loop around
            i = 0;
        }
    }
}
