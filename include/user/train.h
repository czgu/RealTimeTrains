#ifndef _TRAIN_H_
#define _TRAIN_H_

//#include <bqueue.h>

typedef enum { STRAIGHT, CURVED } sw_state;

#define SWITCHCODE 0x20

// Masks for setting switch
#define SMASK 0x1
#define CMASK 0x2

typedef struct Train {
	int id;		// [1,80]
	int speed;	// [0,14]
	int p_spd;	// previous speed
} Train;

int train_cmd(char c1, char c2);
void train_init(Train*, int id);
void train_setspeed(Train*, int speed);

/*
 * Sends reverse command to train and accelerates to previous speed.
 */

void train_reverse(Train*);

#define SWADDRBASEL 1
#define SWADDRBASEH 0x99

#define SWLENL 18
#define SWLENH 4
#define SWLEN (SWLENL + SWLENH)

typedef struct Track_Switch {
	char addr;
	sw_state status;
} Track_Switch;

void switch_init(Track_Switch*, char addr);
// opt = 1 to turn solenoid off after
void train_setswitch(Track_Switch*, sw_state state, int opt);
void train_soloff();


#define SNLEN 5
#define SNSIZE 2
#define SNREQUEST 0xc0

typedef struct Sensor {
	/*
	char lo;
	char hi;*/
	short data;
} Sensor;

void sensor_reset(Sensor*);
void sensor_request(unsigned int module_id);

#endif
