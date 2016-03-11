#include <train.h>
#include <io.h>
#include <syscall.h>

#include <terminal_mvc_server.h>
#include <train_location_server_task.h>

float train_model_speed[15] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    3.745661281,
    4.304676754,
    4.774218154,
    5.286317568,
    5.79537037,
    6.265265265,
    6.303121853
};
float train_model_stop_dist[15] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    561.8,
    643.6,
    714.2,
    797.4,
    856.5,
    962,
    956
};

void train_model_init(TrainModel* train, int id) {
	train->id = id;
	train->speed = 0;
	train->previous_speed = 0;
    train->speed_updated_time = 0;

    train->bitmap = 0;
    train->bitmap |= TRAIN_MODEL_BIT_FWD;

    // useless until the position_known bit is turned on
    train->position.stop_sensor = (void *)0;
    train->position.stop_dist = 0;
}

// Train Model
void train_model_init_location(
    TrainModel* train, 
    int time, 
    short* switches, 
    track_node* sensor_start) 
{
    train->bitmap |= TRAIN_MODEL_BIT_POS;

    // initialize first sensor node
    train->position.dist_travelled = 0;
    train->position.arc = sensor_start->edge + DIR_AHEAD;

    train->position.next_sensor = track_next_sensor_node(switches, train->position.arc, &train->position.estimated_next_sensor_dist);
    
    train->position.updated_time = time;


}

void train_model_update_location(TrainModel* train, int time, short* switches) {
    float delta_dist = 0;
    if (train->bitmap & TRAIN_MODEL_BIT_POS) {
        // TODO: consider acceleration and account speed 0
        if (train->speed > 0) {
            if (train->speed_updated_time > train->position.updated_time) {
                delta_dist += (time - train->speed_updated_time) * train_model_speed[train->speed];
                delta_dist += (train->speed_updated_time - train->position.updated_time) * train_model_speed[train->previous_speed];
            } else {
                delta_dist += (time - train->position.updated_time) * train_model_speed[train->speed];
            }
        } else {
            float stop_time = 300; // TODO: get the actual stop time
            
            // update the remaining part of speed
            if (train->speed_updated_time > train->position.updated_time) {
                delta_dist += (train->speed_updated_time - train->position.updated_time) * train_model_speed[train->previous_speed];
                train->position.updated_time = train->speed_updated_time;
            }

            // update the stopping distance, assume uniform
            if (time - train->speed_updated_time < stop_time) {
                delta_dist += (time - train->position.updated_time)/stop_time * train_model_stop_dist[train->previous_speed];
            }
        }

        // if train is lost
        train->position.estimated_next_sensor_dist -= delta_dist;
        if (train->position.estimated_next_sensor_dist < TRAIN_SENSOR_HIT_TOLERANCE) {
            train->bitmap &= ~TRAIN_MODEL_BIT_POS;
        }

        train->position.dist_travelled += delta_dist;

        train->position.arc = track_next_arc(switches, train->position.arc, &train->position.dist_travelled);

        if (train->position.arc == (void *)0)
            train->bitmap &= ~TRAIN_MODEL_BIT_POS;    

        train->position.updated_time = time;
    }
}

track_edge* track_next_arc(short* switches, track_edge* current, float* dist) {
    while (*dist > current->dist) {
        *dist -= current->dist;

        track_node* node = current->dest;
        switch (node->type) {
            case NODE_MERGE:
            case NODE_SENSOR:
            case NODE_ENTER:
                current = node->edge + DIR_AHEAD;
                break;
            case NODE_BRANCH:
                current = node->edge + switches[node->num];
                break;
            case NODE_EXIT:
            default:
                return (void *)0;
        }
    }
    return current;
}

track_node* track_next_sensor_node(short* switches, track_edge* current, float* dist) {
    *dist = 0;
    track_node* node = current->dest;
    while (node->type != NODE_SENSOR) {
        switch (node->type) {
            case NODE_BRANCH:
                *dist += node->edge[switches[node->num]].dist;
                node = node->edge[switches[node->num]].dest;
            break;
            case NODE_MERGE:
            case NODE_ENTER:
                *dist += node->edge[DIR_AHEAD].dist;
                node = node->edge[DIR_AHEAD].dest;
            break;
            case NODE_EXIT:
            default:
                return 0;
        }
    }
    return node;
}


void train_model_update_speed(TrainModel* train, int time, short* switches, int speed) {
    train_model_update_location(train, time, switches);
    
    train->previous_speed = train->speed;
    train->speed = speed;
    train->speed_updated_time = time;
}

void train_model_reverse_direction(TrainModel* train, int time, short* switches) {
    train_model_update_location(train, time, switches);

    if (train->bitmap & TRAIN_MODEL_BIT_FWD)
        train->bitmap &= ~TRAIN_MODEL_BIT_FWD;
    else 
        train->bitmap |= TRAIN_MODEL_BIT_FWD;

    if (train->bitmap & TRAIN_MODEL_BIT_POS) {
        // Reverse arc

        // TODO: more precise constant based on direction
        train->position.dist_travelled = train->position.arc->dist - train->position.dist_travelled;
        train->position.arc = train->position.arc->reverse;

        train->position.next_sensor = track_next_sensor_node(switches, train->position.arc, &train->position.estimated_next_sensor_dist);
        train->position.estimated_next_sensor_dist -= train->position.dist_travelled;

        if (train->position.next_sensor == 0)
            train->bitmap &= ~TRAIN_MODEL_BIT_POS;

        train->position.updated_time = time; 
    }
}

void train_model_next_sensor_triggered(TrainModel* train, int time, short* switches) {
    // TODO: calculate error
    train_model_update_location(train, time, switches);
    int err = train->position.estimated_next_sensor_dist;

    pprintf(COM2, "\033[%d;%dH", 24 + 8, 1);
    PutStr(COM2, "\033[K");
    pprintf(COM2, "error: %d", err);

    train_model_init_location(train, time, switches, train->position.next_sensor);
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
