#include <train.h>
//#include <trio.h>
//#include <qwrite.h>
#include <io.h>
#include <syscall.h>

void train_init(Train* train, int id) {
	train->id = id;
	train->speed = 0;
	train->p_spd = 0;

	// set train speed to 0
	// note: setting all 80 trains to 0 while delaying each instr by 0.2s would take too long
	//train_setspeed(buffer, train, 0);
}

/*
void delay(struct BQueue* buffer) {
	qputc(buffer, DELAYH);
	qputc(buffer, DELAYL);
}*/

int train_cmd(char c1, char c2) {
    /*
    Putc(COM1, c1);
    Putc(COM1, c2);


    return 0;
    */
    
    //pprintf(COM2, "\033[%d;%dH", 27, 1);
    //pprintf(COM2, "send '%d %d'", c1, c2);

    char msg[2];
    msg[0] = c1;
    msg[1] = c2;
    return PutnStr(COM1, msg, sizeof(char) * 2);
}

void train_setspeed(Train* train, int speed) {
	train->p_spd = train->speed;
	train->speed = speed;

    train_cmd(speed, train->id);
	//qputc(buffer, speed);
	//qputc(buffer, train->id);
	//delay(buffer);
}

void train_reverse(Train* train) {
    train_cmd(15, train->id);
	//qputc(buffer, 15);			// reverse code
	//qputc(buffer, train->id);
	//delay(buffer);

	train_setspeed(train, train->p_spd);
}

void switch_init(Track_Switch* sw, char addr) {
	sw->addr = addr;
	sw->status = CURVED;

	// don't turn solenoid after, since presumably initializing all the switches
	// at once
	train_setswitch(sw, CURVED, 0);
}

void train_setswitch(Track_Switch* sw, sw_state status, int opt) {
	sw->status = status;
    char dir = 0;
	switch (status) {
		case STRAIGHT:
			dir = SWITCHCODE | SMASK;
			break;
		case CURVED:
		    dir = SWITCHCODE | CMASK;
			break;
	}
	//qputc(out, sw->addr);
    train_cmd(dir, sw->addr);
	//delay(out);

	if (opt) {
		train_soloff();
	}
}

void train_soloff() {
	//qputc(out, 32);		// turn off solenoid
    Putc(COM1, 32);         // turn off solenoid
	//delay(out);
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
