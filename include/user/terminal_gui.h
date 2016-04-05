#ifndef _TERMINAL_GUI_H_
#define _TERMINAL_GUI_H_

#define CSTIMEX 15
#define CSTIMEY 1

#define CSSTATSX 1
#define CSSTATSY 2

#define CSSWITCHX 2
#define CSSWITCHY 4
#define CSSWITCHL 9		// col position for left col
#define CSSWITCHR 25	// col position for right col

#define CSTRACKX  55
#define CSTRACKY  3
#define CSSENSORX 55
#define CSSENSORY (CSTRACKY + 2)
//#define SENSOR_DISPLAY_LEN 10
#define MAX_RECENT_SENSORS 10

#define CSTRAINX 1
#define CSTRAIN_HEADERY (CSSWITCHY + 11 + 1)    // 16
#define CSTRAIN_BASEY (CSTRAIN_HEADERY + 1)     // 17

#define CSINPUTX 3
#define CSINPUTY (CSTRAIN_BASEY + 5 + 1)

#define CSDEBUGSTART (CSINPUTY + 2);

#include <rqueue.h>

typedef struct Cursor {
	int row, col;
} Cursor;

typedef struct SensorId {
    char module, id;
} SensorId;

typedef enum {
    NOCOLOUR = 0,
    BLACK = 30,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
} COLOUR;

void init_screen(Cursor* cs);

// struct Switch* switches, int nswitches);

void print_clr();
void print_backsp(Cursor* cs);
//void print_msg(Cursor* cs, char* msg);      // deprecated
void reset_cursor(Cursor* cs);

// Part-specific
void print_time(Cursor* cs, unsigned int ticks);
void print_switch(Cursor* cs, char switch_status, int index);
void print_stats(Cursor* cs, short percent);
void print_sensor(Cursor* cs, int index, SensorId sensor);
void print_track(Cursor* cs, int track);

#define MAXDEBUGMSGLEN 200
void print_debug(Cursor* cs, char* fmt, ...);

// warning: use at your own risk
// The function does not know the state of the cursor, so it
// cannot restore it at the end. This can have concurrency
// issues with other print statements.
void debugf(char* fmt, ...);
void debugc(COLOUR colour, char* fmt, ...);

typedef enum {
    TRAIN_ID = 0,
    TRAIN_SPEED,
    TRAIN_ACCEL_STATE,
    TRAIN_ARC,
    TRAIN_DISTANCE_TRAVELLED,
    TRAIN_NEXT_SENSOR,
    TRAIN_LOCATION_ERR,
    TRAIN_DESTINATION,
    TRAIN_TRACK_ALLOC,
    TRAIN_TRACK_DEALLOC,
    TRAIN_TRACK_STATE,           // variable number of tabs
} TRAIN_DISPLAY_HEADER;

#define MAX_DISPLAY_TRAINS 5
#define TRAIN_NUM_COLS 11       // number of elements in TRAIN_DISPLAY_HEADER
#define TAB_LENGTH 8

/*
    print_train_bulk: prints formatted string for columns in [col_start, col_end]
    train_row: [0, MAX_DISPLAY_TRAINS)
*/
void print_train_bulk(Cursor* cs, int train_row, 
                      TRAIN_DISPLAY_HEADER col_start, TRAIN_DISPLAY_HEADER col_end,
                      char* fmt, ...);
void clear_train_row(Cursor* cs, int train_row);
#endif
