#ifndef _TERMINAL_GUI_H_
#define _TERMINAL_GUI_H_

#define CSTIMEX 15
#define CSTIMEY 1

#define CSSWITCHX 2
#define CSSWITCHY 4
#define CSSWITCHL 9		// col position for left col
#define CSSWITCHR 25	// col position for right col

#define CSSENSORX 55
#define CSSENSORY 4

#define CSINPUTX 3
#define CSINPUTY 20

#define CSMSGX 1
#define CSMSGY 24

typedef struct Cursor {
	int row, col;
} Cursor;

void init_screen(Cursor* cs);

// struct Switch* switches, int nswitches);

void print_clr();
void print_backsp(Cursor* cs);
void print_msg(Cursor* cs, char* msg);
void reset_cursor(Cursor* cs);

// Part-specific
void print_time(Cursor* cs, unsigned int ticks);
void print_switch(Cursor* cs); // struct Switch, int index);
void print_sensor(Cursor* cs);//, struct BQueue64* recent);

#endif
