#include <terminal_gui.h>
#include <train_logic_task.h>
#include <terminal_mvc_server.h>

#include <syscall.h>
#include <io.h>

void calibrate_stop() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    int command_server_tid = WhoIs("Command Server");

    train_set_speed(command_server_tid, train_id, command.param[2]);

    TERMmsg msg;
    msg.opcode = CMD_SENSOR_MODULE_AWAIT;
    msg.param[0] = command.param[3]; // module C
    
    short sensor_bitmap;

    for (;;) {
        Send(command_server_tid, &msg, sizeof(TERMmsg), &sensor_bitmap, sizeof(short));
        if (sensor_bitmap & (0x8000 >> (command.param[4] - 1)))
            break;
    }

    train_set_speed(command_server_tid, train_id, 0);
}

void calibrate_stop_time() {
    

}

void calibrate_velocity() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    int command_server_tid = WhoIs("Command Server");


    TERMmsg msgs[2];

    msgs[0].opcode = CMD_SENSOR_MODULE_AWAIT;
    msgs[1].opcode = CMD_SENSOR_MODULE_AWAIT;

    msgs[0].param[0] = 4; // module D
    msgs[1].param[0] = 5; // module E


    short sensor_bitmap;
    int turn = 0;
    int tick = 0;

    int tries = 0;
    int line = 24;

    //set_track(command_server_tid, 11, 1);

    int speed = command.param[2];
    for (;;) {
        train_set_speed(command_server_tid, train_id, speed);
        turn = 0;
        tries = 0;

        for (;;) {
            Send(command_server_tid, &msgs[turn], sizeof(TERMmsg), &sensor_bitmap, sizeof(short));

            if (turn == 0) {
                if (sensor_bitmap & (0x8000 >> (5 - 1))) {
                    tick = Time();
                    turn ++;
                }
            } else { // turn 1
                if (sensor_bitmap & (0x8000 >> (3 - 1))) {
                    pprintf(COM2, "\033[%d;%dH", line + tries, 1 + (speed - 8) * 8);
                    pprintf(COM2, "%d:%d", speed,Time() - tick);
                    turn = 0;
                    tries ++;
                }
            }
            if (tries > 10)
                break;
        }
        speed++;
        if (speed > command.param[3])
            break;
    }

    train_set_speed(command_server_tid, train_id, 0);

}

void calibrate_acceleration_delta() {

}

void calibrate_acceleration_move() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    int command_server_tid = WhoIs("Command Server");

    train_set_speed(command_server_tid, train_id, command.param[2]);

    Delay(command.param[3]);

    train_set_speed(command_server_tid, train_id, 0);

}
