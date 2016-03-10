#include <terminal_gui.h>

#include <io.h>
#include <syscall.h>

// for constants
#include <train_logic_task.h>
#include <track_data.h>
#include <train.h>

void init_time() {
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

void init_sensors() {
	pprintf(COM2, "\033[%d;%dH", CSSENSORY - 1, CSSENSORX);
	PutStr(COM2, "Recent sensors:");
	
    /*
	int i;
	for(i = 0; i < 5; i++) {
		pprintf(COM2, "\033[%d;%dH", CSSENSORY + i, CSSENSORX);
		pprintf(COM2, "%d.", i + 1);
	}
	}*/
}

void init_screen(Cursor* cs) { //, struct Switch* sws, int nsw) {
    PutStr(COM2, "\033[?25l");
	print_clr();				// clear screen

	init_time();
	init_switches(); //, sws, nsw);
	init_sensors();

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
	pprintf(COM2, "idle: %d.%d ", percent/10, percent%10);
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	// move cursor back to original position
}


void print_clr() {
	pprintf(COM2, "\033[2J");			// clear screen
}

void print_backsp(Cursor* cs) {
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);
	PutStr(COM2, "\033[K");
}

void print_msg(Cursor* cs, char* msg) {
    static int i = 0;

	pprintf(COM2, "\033[%d;%dH", CSMSGY + i, CSMSGX);
	PutStr(COM2, "\033[K");				// clear previous message
	PutStr(COM2, msg);
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);

    if (++i > 15)
        i = 0;

}

void reset_cursor(Cursor* cs) {
	cs->row = CSINPUTY;
	cs->col = CSINPUTX;
	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);
	pprintf(COM2, "\033[K");
}

void print_switch(Cursor* cs, char switch_status, int index) {
    index --;
	int col = (index < 12)? CSSWITCHL : CSSWITCHR;
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

//TODO: need a better way, maybe just pass index of the track is good
void print_landmark(int type, int num) {
    switch(type) {
        case NODE_SENSOR:
            pprintf(COM2, "%c%d", num / 16 + 'A', num % 16 + 1);
            break;
        case NODE_BRANCH:
            pprintf(COM2, "BR%d", num);
            break;
        case NODE_MERGE:
            pprintf(COM2, "MR%d", num);
            break;
        case NODE_ENTER:
            pprintf(COM2, "EN%d", num);
            break;
        case NODE_EXIT:
            pprintf(COM2, "EX%d", num);
            break;
        
    }
}

void print_train(Cursor* cs, int train, int src_type, int src_num, int dest_type, int dest_num, int dist) {
    pprintf(COM2, "\033[%d;%dH", CSTRAINY + train - TRAIN_ID_MIN, CSTRAINX);
    PutStr(COM2, "\033[K"); // clear previous message

    pprintf(COM2, "%d: ", train);
    print_landmark(src_type, src_num);
    PutnStr(COM2, " - ", 3);
    print_landmark(dest_type, dest_num);

    PutnStr(COM2, ", dist: ", 8);
    pprintf(COM2, "%d", dist);

	pprintf(COM2, "\033[%d;%dH", cs->row, cs->col);	// move cursor back to original position
    
}


