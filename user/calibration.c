#include <terminal_gui.h>
#include <train_logic_task.h>
#include <train_location_server_task.h>
#include <terminal_mvc_server.h>

#include <train.h>
#include <syscall.h>
#include <io.h>

// for debugf
#include <terminal_gui.h>

void calibrate_stop();

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
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    int min_spd = command.param[2];
    int max_spd = command.param[3];
    int loc_server = WhoIs("Location Server");

    // mm
    double stop_distances[15] = { 0 };
    switch (train_id) {
        case 62:
            stop_distances[8] = 482.8;
            stop_distances[9] = 533.6;
            stop_distances[10] = 594.8;
            stop_distances[11] = 665.75;
            stop_distances[12] = 745.25;
            stop_distances[13] = 808.75;
            stop_distances[14] = 795.5;
            break;
        case 63:
            stop_distances[8] = 561.8;
            stop_distances[9] = 643.6;
            stop_distances[10] = 714.2;
            stop_distances[11] = 797.4;
            stop_distances[12] = 856.5;
            stop_distances[13] = 962;
            stop_distances[14] = 956;
            break;
        default:
            break;
    }
    double tol = 27.0;                  // distance from front of train to pickup
    double dist_d8_e8 = 375.0;
    
    //double dist_e8_c14 = 877.0;         // track A
    double dist_e8_c14 = 785.0;         // track B

    int speed, trial;
    for (speed = min_spd; speed <= max_spd; speed++) {
        for (trial = 0; trial < 10; trial++) {
            train_set_speed(loc_server, train_id, speed);

            wait_sensor(loc_server, 4, 8);
            int time0 = Time();

            wait_sensor(loc_server, 5, 8);
            int time1 = Time();

            double velocity = dist_d8_e8 / (time1 - time0);
            double wait_time = 
                (tol + dist_e8_c14 - stop_distances[speed]) / velocity;
            Delay(wait_time);

            train_set_speed(loc_server, train_id, 0);
            int time2 = Time();

            wait_sensor(loc_server, 3, 14);
            int time3 = Time();

            debugf("%d:%d", speed, time3 - time2);

            Delay(200);
        }
    }
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

    //set_track(command_server_tid, 11, 1);

    track_set_switch(location_server_tid, 20, CURVED, 0);
    track_set_switch(location_server_tid, 19, STRAIGHT, 0);
    track_set_switch(location_server_tid, 10, CURVED, 0);
    track_set_switch(location_server_tid, 9, CURVED, 1);

    int increase = 1;
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
                debugf("%d:%d", speed, Time() - tick);
                turn = 0;
                tries ++;
            }
            if (tries > 10)
                break;
        }
        if (increase == 1) {
            speed++;
        } else {
            speed--;
        }
        if (speed > command.param[3]) {
            speed = speed - 2;
            increase = 0;
        }
        if (speed < command.param[2]) {
            break;
        }
    }

    train_set_speed(location_server_tid, train_id, 0);
}

void calibrate_acceleration_delta() {
    // C10 -> B3 -> C2
    
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    // user should input these values
    int train_id = command.param[1];
    int train_speed[2];
    train_speed[0] = command.param[2];
    train_speed[1] = command.param[3];

    int loc_server = WhoIs("Location Server");

    // make switches 13, 14 curved
    track_set_switch(loc_server, 20, STRAIGHT, 0);
    track_set_switch(loc_server, 19, CURVED, 0);
    track_set_switch(loc_server, 13, CURVED, 0);
    track_set_switch(loc_server, 14, CURVED, 1);

    int i;
    int time[6];

    for (i = 0; i < 10; i++) {
        train_set_speed(loc_server, train_id, train_speed[0]);

        // Wait for train to trigger B15
        wait_sensor(loc_server, 2, 15);
        time[0] = Time();

        // Wait for train to trigger A3
        wait_sensor(loc_server, 1, 3);
        time[1] = Time();

        /*
        if (i != 0) {
            Delay(10 * i);              // adjustable
        }*/
        train_set_speed(loc_server, train_id, train_speed[1]);
        time[2] = Time();

        // Wait for train to trigger C11
        wait_sensor(loc_server, 3, 11);
        time[3] = Time();
        

        // Wait for train to trigger E16
        wait_sensor(loc_server, 5, 16);
        time[4] = Time();

        // Wait for train to trigger E1
        wait_sensor(loc_server, 5, 1);
        time[5] = Time();

        // print: time 1->2:(ticks):time 2->3
        debugf("(%d, %d, %d, %d, %d)", 
                time[1] - time[0], 
                time[2] - time[1], 
                time[3] - time[2], 
                time[4] - time[3], 
                time[5] - time[4]);

        // Delay 0.5 seconds
        Delay(50);
    }
    train_set_speed(loc_server, train_id, 0);
}

void calibrate_acceleration_move() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    debugf("calib");
    int train_id = command.param[1];
    //int command_server_tid = WhoIs("Command Server");
    int location_server_tid = WhoIs("Location Server");

    train_set_speed(location_server_tid, train_id, command.param[2]);

    int time1 = Time();

    Delay(command.extra);
    int time2 = Time();

    train_set_speed(location_server_tid, train_id, 0);
    debugf("dt: (%d, %d)", command.extra, time2 - time1);
}
void calibrate_find_train() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    int train_id = command.param[1];
    int location_server_tid = WhoIs("Location Server");

    TrainModelPosition position;
    train_set_speed(location_server_tid, train_id, 10);
    while (where_is(location_server_tid, train_id, &position) == 0) {
        Delay(100);
    }

    train_set_speed(location_server_tid, train_id, 0);
}


