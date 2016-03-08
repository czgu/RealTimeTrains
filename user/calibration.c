#include <terminal_gui.h>
#include <train_logic_task.h>
#include <train_location_server_task.h>
#include <terminal_mvc_server.h>

#include <train.h>
#include <syscall.h>
#include <io.h>

void calibrate_stop() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    // int command_server_tid = WhoIs("Command Server");
    int location_server_tid = WhoIs("Location Server");

    train_set_speed(location_server_tid, train_id, command.param[2]);

    wait_sensor(location_server_tid, command.param[3], command.param[4]);

    train_set_speed(location_server_tid, train_id, 0);
}

void calibrate_stop_time() {
    

}

void calibrate_velocity() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    int location_server_tid = WhoIs("Location Server");
    // int command_server_tid = WhoIs("Command Server");

    // short sensor_bitmap;
    int turn = 0;
    int tick = 0;

    int tries = 0;
    int line = 24;

    //set_track(command_server_tid, 11, 1);

    int speed = command.param[2];
    for (;;) {
        train_set_speed(location_server_tid, train_id, speed);
        turn = 0;
        tries = 0;

        for (;;) {
            if (turn == 0) {
                wait_sensor(location_server_tid, 4, 5);
                tick = Time();
                turn ++;
            } else { // turn 1
                wait_sensor(location_server_tid, 5, 3);
                pprintf(COM2, "\033[%d;%dH", line + tries, 1 + (speed - 8) * 8);
                pprintf(COM2, "%d:%d", speed,Time() - tick);
                turn = 0;
                tries ++;
            }
            if (tries > 10)
                break;
        }
        speed++;
        if (speed > command.param[3])
            break;
    }

    train_set_speed(location_server_tid, train_id, 0);
}

void calibrate_acceleration_delta() {

}

void calibrate_acceleration_move() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    //int command_server_tid = WhoIs("Command Server");
    int location_server_tid = WhoIs("Location Server");

    train_set_speed(location_server_tid, train_id, command.param[2]);

    Delay(command.param[3]);

    train_set_speed(location_server_tid, train_id, 0);

}
