#include <terminal_gui.h>

#include <assert.h>
#include <io.h>
#include <syscall.h>

// for constants
#include <train_logic_task.h>
#include <track_data.h>
#include <train.h>

char TRAIN_TAB_STOP[TRAIN_NUM_COLS + 1] = {0, 1, 2, 4, 5, 6, 7, 8, 15};

inline void init_time() {
	PutStr(COM2, "\033[H");					// move cursor to top-left
	PutStr(COM2, "Time elapsed:");
}

void init_switches() {
	pprintf(COM2, "\033[%d;%dH", CSSWITCHY - 1, CSSWITCHX);
	PutStr(COM2, "Switches:");

	int i;
	int half = NUM_TRAIN_SWITCH / 2;
	for (i = 0; i < half; i++) {
		pprintf(COM2, "\033[%d;%dH", CSSWITCHY + i, CSSWITCHX);
		pprintf(COM2, "%d:\tS\t%d:\tS", i + 1, i + half + 1);
	}
    
}

inline void init_sensors() {
	pprintf(COM2, "\033[%d;%dH", CSSENSORY - 1, CSSENSORX);
	PutStr(COM2, "Recent sensors:");

    // initialize scrolling window for sensors
    pprintf(COM2, "\033[%d;%dr", CSSENSORY, CSSENSORY + MAX_RECENT_SENSORS);
	
    /*
	int i;
	for(i = 0; i < 5; i++) {
		pprintf(COM2, "\033[%d;%dH", CSSENSORY + i, CSSENSORX);
		pprintf(COM2, "%d.", i + 1);
	}
	}*/
}

inline void init_track_display() {
    pprintf(COM2, "\033[%d;%dH", CSTRACKY, CSTRACKX);
    PutnStr(COM2, "Track ?", 7);
}

inline void init_train_display() {
	pprintf(COM2, "\033[%d;%dH", CSTRAIN_HEADERY, CSTRAINX);
    PutStr(COM2, "Train\tSpeed\tCurrent arc\tDstance\tN.sensr\tError\tDest\tTrack state");
}

inline void init_debug_display() {
    // init scrolling region
    pprintf(COM2, "\033[%d;r", CSINPUTY + 2);    // define scroll region to the bottom
}

void init_screen(Cursor* cs) { //, struct Switch* sws, int nsw) {
    pprintf(COM2, "\033c");     // reset terminal display settings
    PutStr(COM2, "\033[?25l");  // hide the cursor
	print_clr();				// clear screen

	init_time();
	init_switches(); //, sws, nsw);
	init_sensors();
    init_track_display();
    init_train_display();
    init_debug_display();

	pprintf(COM2, "\033[%d;%dH>", CSINPUTY, 1);
	reset_cursor(cs);		// set cursor to user input position
}

void print_time(Cursor* cs, unsigned int ticks) {
	unsigned int min, sec;

    // 1 tick = 10 ms

    min = ticks / (100 * 60);
    ticks %= (100 * 60);

    sec = ticks / 100;
    ticks %= 100;

	pprintf(COM2, "\033[%d;%dH", CSTIMEY, CSTIMEX);
	//qprintf(out, "%s", "\033[H");					// move cursor to top-left
	PutStr(COM2, "\033[K");							// clear previous message
	pprintf(COM2, "%d:%d:%d", min, sec, ticks / 10);
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	// move cursor back to original position
}

void print_stats(Cursor* cs, short percent) {
	pprintf(COM2, "\033[%d;%dH", CSSTATSY, CSSTATSX);
	PutStr(COM2, "\033[K");							// clear previous message
	pprintf(COM2, "Idle: %d.%d ", percent/10, percent%10);
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	// move cursor back to original position
}

void print_debug(Cursor* cs, char* fmt, ...) {
    // pretty format input
    va_list va;
    va_start(va, fmt);

    char output_buffer[MAXDEBUGMSGLEN];
    int string_len;
    pretty_format(fmt, va, output_buffer, MAXDEBUGMSGLEN, &string_len);
    va_end(va);

    PutStr(COM2, "\033F\033D");                 // move cursor down and scroll down
    PutnStr(COM2, output_buffer, string_len);

	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);     // move cursor back to original position
}

void debugf(char* fmt, ...) {
    // pretty format input
    va_list va;
    va_start(va, fmt);

    char output_buffer[MAXDEBUGMSGLEN];
    int string_len;
    pretty_format(fmt, va, output_buffer, MAXDEBUGMSGLEN, &string_len);
    va_end(va);

    // save cursor
    // hopefully this solves some concurrent print cursor issues,
    // but competing debug prints will still err
    //PutStr(COM2, "\0337");

    // print debug message
    // 1. save cursor               \0337
    // 2. move cursor to lower left \033F
    // 3. scroll down               \033D
    // 4. print message
    // 5. restore cursor            \0338
    pprintf(COM2, "\0337\033F\033D%s\0338", output_buffer);

    // restore cursor
    //PutStr(COM2, "\0338");
}

void debugc(COLOUR colour, char* fmt, ...) {
    ASSERTP(BLACK <= colour && colour <= WHITE, "invalid colour %d", colour);
    // pretty format input
    va_list va;
    va_start(va, fmt);

    char output_buffer[MAXDEBUGMSGLEN];
    int string_len;
    pretty_format(fmt, va, output_buffer, MAXDEBUGMSGLEN, &string_len);
    va_end(va);

    // print debug message
    // 0. set colour                \033[%dm
    // 1. save cursor               \0337
    // 2. move cursor to lower left \033F
    // 3. scroll down               \033D
    // 4. print message
    // 5. restore cursor            \0338
    // 6. reset colour              \033[0m
    pprintf(COM2, "\033[%dm\0337\033F\033D%s\0338\033[%dm", 
            colour, output_buffer, NOCOLOUR);
}

void print_clr() {
	pprintf(COM2, "\033[2J");			// clear screen
}

void print_backsp(Cursor* cs) {
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);
	PutStr(COM2, "\033[K");
}

/*
void print_msg(Cursor* cs, char* msg) {
    static int i = 0;

	pprintf(COM2, "\033[%d;%dH", CSMSGY + i, CSMSGX);
	PutStr(COM2, "\033[K");				// clear previous message
	PutStr(COM2, msg);
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);

    if (++i > 15)
        i = 0;

}*/

void reset_cursor(Cursor* cs) {
	cs->row = CSINPUTY;
	cs->col = CSINPUTX;
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);
	pprintf(COM2, "\033[K");
}

void reset_text_format() {
    pprintf(COM2, "\033[0m");
}

void print_switch(Cursor* cs, char switch_status, int index) {
    index--;
	int col = (index < 11)? CSSWITCHL : CSSWITCHR;
	pprintf(COM2, "\033[%d;%dH", CSSWITCHY + (index % 11), col);

	switch(switch_status) {
		case DIR_STRAIGHT:
			//qputc(out, 'S');
			Putc(COM2, 'S');
			break;
		case DIR_CURVED:
			Putc(COM2, 'C');
			break;
	}
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	// move cursor back to original position
}

void print_sensor(Cursor* cs, int index, SensorId sensor) {
    pprintf(COM2, "\033[%d;%dH", CSSENSORY + index, CSSENSORX);
    PutStr(COM2, "\033[K"); // clear previous message

    pprintf(COM2, "%d. %c%d", index + 1, 
            sensor.module + 'A', sensor.id);
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	// move cursor back
}

void print_track(Cursor* cs, int track) {
    pprintf(COM2, "\033[%d;%dH", CSTRACKY, CSTRACKX);
    PutStr(COM2, "\033[KTrack "); // clear previous message
    switch (track) {
        case TRACK_A:
            PutStr(COM2, "A");
            break;
        case TRACK_B:
            PutStr(COM2, "B");
            break;
        default:
            // invalid track parameter
            pprintf(COM2, "unknown %d", track);
            break;
    }

    // move cursor back to original position
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);
}

void clear_train_row(Cursor* cs, int train_row) {
    ASSERTP(0 <= train_row && train_row < MAX_DISPLAY_TRAINS, 
            "invalid row index %d", train_row);
     
    pprintf(COM2, "\033[%d;%dH", CSTRAIN_BASEY + train_row, CSTRAINX);
    pprintf(COM2, "\033[K");

    // move cursor back to original position
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	
}

void print_train_bulk(Cursor* cs, int train_row, 
                      TRAIN_DISPLAY_HEADER col_start, TRAIN_DISPLAY_HEADER col_end,
                      char* fmt, ...) {
    ASSERTP(TRAIN_ID <= col_start && col_start <= TRAIN_TRACK_STATE, 
            "invalid column %d", col_start);
    ASSERTP(TRAIN_ID <= col_end && col_end <= TRAIN_TRACK_STATE, 
            "invalid column %d", col_end);
    ASSERTP(col_start <= col_end, 
            "invalid range: [%d, %d]", col_start, col_end);
    ASSERTP(0 <= train_row && train_row < MAX_DISPLAY_TRAINS, 
            "invalid row index %d", train_row);
    int i;
    // pretty format input
    va_list va;
    va_start(va, fmt);

    char output_buffer[100];
    int string_len;
    pretty_format(fmt, va, output_buffer, 100, &string_len);
    va_end(va);
    
    pprintf(COM2, "\033[%d;%dH", CSTRAIN_BASEY + train_row, CSTRAINX);
    for(i = 0; i < TRAIN_TAB_STOP[col_start]; i++) {
        PutStr(COM2, "\033[I");                                     // tab forward
    }
    int num_tabs = TRAIN_TAB_STOP[col_end + 1] - TRAIN_TAB_STOP[col_start];
    pprintf(COM2, "\033[%dX", TAB_LENGTH * num_tabs);               // clear cols

    pprintf(COM2, "\033[%dm", GREEN + train_row);  // set colour
    PutnStr(COM2, output_buffer, string_len);
    PutStr(COM2, "\033[0m");                                        // reset colour

    // move cursor back to original position
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	
}
