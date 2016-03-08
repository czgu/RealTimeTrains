#ifndef _TRAIN_H_
#define _TRAIN_H_

//#include <bqueue.h>

typedef enum { STRAIGHT = 0, CURVED } sw_state;

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
void train_set_speed(int location_server_tid, int train, int speed);

/*
 * Sends reverse command to train
 */
void train_reverse(int location_server_tid, int train);


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
void track_set_switch(int location_server_tid, int track_switch, char curve);
void track_soloff();


#define SNLEN 5
#define SNSIZE 2
#define SNREQUEST 0xc0
#define MULTI_SNREQUEST 0x80
typedef struct SensorData {
	char lo;
	char hi;
	//short data;
    // WARNING: If more data is added here, need to modify train_sensor.c
} SensorData;

void sensor_reset(SensorData*);
void sensor_request(unsigned int module_id);
void sensor_request_upto(unsigned int module_id);


#endif
