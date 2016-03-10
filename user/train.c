#include <train.h>
#include <io.h>
#include <syscall.h>

#include <terminal_mvc_server.h>
#include <train_location_server_task.h>

void train_init(Train* train, int id) {
	train->id = id;
	train->speed = 0;
	train->p_spd = 0;
}

inline int train_cmd(char c1, char c2) {
    char msg[3];
    msg[0] = c1;
    msg[1] = c2;
    return PutnStr(COM1, msg, sizeof(char) * 2);
}

void train_set_speed(int location_server_tid, int train, int speed) {
    TERMmsg msg;
    msg.opcode = LOC_TRAIN_SPEED_UPDATE;
    msg.param[0] = train;
    msg.param[1] = speed;    

    Send(location_server_tid, &msg, sizeof(TERMmsg), 0, 0);

    train_cmd(speed, train);
}

void train_reverse(int location_server_tid, int train) {
    TERMmsg msg;
    msg.opcode = LOC_TRAIN_SPEED_REVERSE_UPDATE;
    msg.param[0] = train;

    Send(location_server_tid, &msg, sizeof(TERMmsg), 0, 0);

    train_cmd(15, train);
}

void switch_init(Track_Switch* sw, char addr) {
	sw->addr = addr;
	sw->status = CURVED;

	// don't turn solenoid after, since presumably initializing all the switches
	// at once
}

void track_soloff() {
	//qputc(out, 32);		// turn off solenoid
    Putc(COM1, 32);         // turn off solenoid
	//delay(out);
}

void train_switch_task() {
    // turn off solenoid
    Delay(15);
    track_soloff();
}

void track_set_switch(int location_server_tid, int track_switch, char curve, int soloff) {
    TERMmsg msg;
    msg.opcode = LOC_SWITCH_UPDATE;
    msg.param[0] = track_switch;
    msg.param[1] = curve;    

    Send(location_server_tid, &msg, sizeof(TERMmsg), 0, 0);

    char dir = (curve == STRAIGHT) ? SWITCHCODE | SMASK : SWITCHCODE | CMASK;

    train_cmd(dir, track_switch);

    if (soloff) {
        Create(30, train_switch_task);
    }
}

void sensor_reset(SensorData* sn) {
	sn->lo = 0;
	sn->hi = 0;
	//sn->data = 0;
}

void sensor_request(unsigned int module_id) {
	//qputc(out, SNREQUEST | module_id);
    Putc(COM1, SNREQUEST | module_id);
	//delay(out);
}

void sensor_request_upto(unsigned int module_id) {
    Putc(COM1, MULTI_SNREQUEST | module_id);
}
