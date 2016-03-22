#include <train.h>
#include <io.h>
#include <syscall.h>
#include <priority.h>

#include <terminal_mvc_server.h>
#include <train_location_server_task.h>

void train_calibration_profile_init(TrainCalibrationProfile* profile, int id) {
    int i;
    for (i = 0; i < 8; i++) {
        profile->velocity[i] = 0;
        profile->stop_distance[i] = 0;
    }
    /*
    for (i = 0; i < 15; i++) {
        profile->calibration_weight[i] = 0.2;
    }*/
    switch(id) {
        // TODO: Add more train calibration data
        case 63:
        default:
            profile->velocity[8]  = 3.745661281;
            profile->velocity[9]  = 4.304676754;
            profile->velocity[10] = 4.774218154;
            profile->velocity[11] = 5.286317568;
            profile->velocity[12] = 5.79537037;
            profile->velocity[13] = 6.265265265;
            profile->velocity[14] = 6.303121853;

            profile->stop_distance[8] = 561.8;
            profile->stop_distance[9] = 643.6;
            profile->stop_distance[10] = 714.2;
            profile->stop_distance[11] = 797.4;
            profile->stop_distance[12] = 856.5;
            profile->stop_distance[13] = 962;
            profile->stop_distance[14] = 956;
    }
}

void train_model_init(TrainModel* train, int id) {
	train->id = id;
	train->speed = 0;
	train->previous_speed = 0;
    train->speed_updated_time = 0;
    train->velocity = 0;
    train->accel_const = 0;

    train->bitmap = 0;
    train->bitmap |= TRAIN_MODEL_DIRECTION_FWD;

    // useless until the position_known bit is turned on
    train->position.stop_node = (void *)0;
    train->position.stop_dist = 0;

    train_calibration_profile_init(&train->profile, id);
}

// Train Model
void train_model_init_location(
    TrainModel* train, 
    int time, 
    short* switches, 
    track_node* sensor_start) 
{
    train->bitmap |= TRAIN_MODEL_POSITION_KNOWN;

    // initialize first sensor node
    //train->position.prev_sensor_dist = 0;
    train->position.sensor_triggered_time = 0;

    train->position.dist_travelled = 0;
    train->position.arc = sensor_start->edge + DIR_AHEAD;
    train->position.prev_arc = 0;

    train->position.next_sensor = track_next_sensor_node(switches, train->position.arc, &train->position.estimated_next_sensor_dist);
    
    train->position.updated_time = time;
}

void train_model_update_location(TrainModel* train, int time, short* switches) {
    float delta_dist = 0;
    if (train->accel_const != 0) {
        // train is accelerating
        train->velocity = train->velocity + train->accel_const * TRAIN_ACCELERATION_DELTA;
        // TODO: I'm not sure how good floating point operations are
        if (train->accel_const * train->velocity >= train->accel_const * train->profile.velocity[train->speed]) {
            train->accel_const = 0;
            train->velocity = train->profile.velocity[train->speed];
            //pprintf(COM2, "\033[%d;%dH\033[K[%d] (done acceleration)\n\r", 
            //    24 + 19, 1, Time());
        }
    }
    if (train->bitmap & TRAIN_MODEL_POSITION_KNOWN) {
        float vel_multiplier = 1.0 / train->position.arc->weight_factor;
        // TODO: consider acceleration and account speed 0
#if 1
        delta_dist += (time - train->position.updated_time) * train->velocity * vel_multiplier;
#else
        if (train->speed > 0) {
            /*
            if (train->speed_updated_time > train->position.updated_time) {
                delta_dist += (time - train->speed_updated_time) 
                            * train->velocity * vel_multiplier;
                delta_dist += (train->speed_updated_time - train->position.updated_time) 
                            * train->velocity * vel_multiplier;
            } else {*/
            delta_dist += (time - train->position.updated_time) * train->velocity * vel_multiplier;
            //}
        } else {
            float stop_time = 300; // TODO: get the actual stop time
            
            // update the remaining part of speed
            if (train->speed_updated_time > train->position.updated_time) {
                delta_dist += (train->speed_updated_time - train->position.updated_time) 
                            * train->velocity * vel_multiplier;
                train->position.updated_time = train->speed_updated_time;
            }

            // update the stopping distance, assume uniform
            if (time - train->speed_updated_time < stop_time) {
                delta_dist += (time - train->position.updated_time)/stop_time 
                            * train->profile.stop_distance[train->previous_speed];
            }
        }
#endif
        // if train is lost
        train->position.estimated_next_sensor_dist -= delta_dist;
        if (train->position.estimated_next_sensor_dist < TRAIN_SENSOR_HIT_TOLERANCE) {
            train->bitmap &= ~TRAIN_MODEL_POSITION_KNOWN;
        }

        train->position.dist_travelled += delta_dist;

        train->position.arc = track_next_arc(switches, train->position.arc, &train->position.dist_travelled);

        if (train->position.arc == (void *)0)
            train->bitmap &= ~TRAIN_MODEL_POSITION_KNOWN;

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
            case NODE_BRANCH: {
                int switch_num = node->num;
                if (switch_num >= 153) {
                    switch_num -= (153 - 19);
                }
                current = node->edge + switches[switch_num];
                break;
            }
            case NODE_EXIT:
            default:
                return (void *)0;
        }
    }
    return current;
}

track_node* track_next_sensor_node(short* switches, track_edge* current, float* dist) {
    *dist = current->dist;
    track_node* node = current->dest;
    while (node->type != NODE_SENSOR) {
        switch (node->type) {
            case NODE_BRANCH: {
                int switch_num = node->num;
                if (switch_num >= 153) {
                    switch_num -= (153 - 19);
                }
                *dist += node->edge[switches[switch_num]].dist;
                node = node->edge[switches[switch_num]].dest;
            break;
            }
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

int track_ahead_contain_node(
    track_node* node, short* switches, track_node* current, 
    float lookahead_dist, float *dist_to_node) {

    *dist_to_node = 0;
    while (node != current) {
        track_edge* next_arc;
        switch (current->type) {
            case NODE_MERGE:
            case NODE_SENSOR:
            case NODE_ENTER:
                next_arc = current->edge + DIR_AHEAD;
                break;
            case NODE_BRANCH: {
                int switch_num = node->num;
                if (switch_num >= 153) {
                    switch_num -= (153 - 19);
                }
                next_arc = current->edge + switches[switch_num];
                break;
            }
            case NODE_EXIT:
            default: // can't search no more
                return 0;
        }

        lookahead_dist -= next_arc->dist;
        *dist_to_node += next_arc->dist;

        if (lookahead_dist < 0)
            break;

        current = next_arc->dest;
        
    }
    return node == current;   
}

void train_model_update_speed(TrainModel* train, int time, short* switches, int speed) {
    train_model_update_location(train, time, switches);
    
    if (speed != train->speed) {
        train->previous_speed = train->speed;
        train->speed = speed;
        train->speed_updated_time = time;
    
        train->accel_const = (train->previous_speed < train->speed)? 1 : -1;
        //pprintf(COM2, "\033[%d;%dH\033[K[%d] (start acceleration)\n\r", 
        //    24 + 19, 1, Time());
    }
}

void train_model_reverse_direction(TrainModel* train, int time, short* switches) {
    train_model_update_location(train, time, switches);

    if (train->bitmap & TRAIN_MODEL_DIRECTION_FWD)
        train->bitmap &= ~TRAIN_MODEL_DIRECTION_FWD;
    else 
        train->bitmap |= TRAIN_MODEL_DIRECTION_FWD;

    if (train->bitmap & TRAIN_MODEL_POSITION_KNOWN) {
        // Reverse arc

        // TODO: more precise constant based on direction
        train->position.dist_travelled = train->position.arc->dist - train->position.dist_travelled;
        train->position.arc = train->position.arc->reverse;

        train->position.next_sensor = track_next_sensor_node(switches, train->position.arc, &train->position.estimated_next_sensor_dist);
        train->position.estimated_next_sensor_dist -= train->position.dist_travelled;

        if (train->position.next_sensor == 0)
            train->bitmap &= ~TRAIN_MODEL_POSITION_KNOWN;

        train->position.updated_time = time; 
    }
}

//int line = 1;
void train_model_next_sensor_triggered(TrainModel* train, int time, short* switches) {
    // dynamically calibrate velocity and track
    // We want to wait for the train to accelerate enough after a change of speed before calibration
    if (train->accel_const == 0 && train->position.prev_arc == train->position.arc) {
        int time_delta = time - train->position.sensor_triggered_time;
        int distance = train->position.prev_arc->dist;
        float agg_velocity = (float) distance / time_delta;
        float velocity_old = train->profile.velocity[train->speed];
        float arc_weight_old = train->position.prev_arc->weight_factor;

        if (distance > 0) {
            train->position.prev_arc->weight_factor = arc_weight_old * 0.9
                + train->profile.velocity[train->speed] * time_delta / distance * 0.1;
        }
        if (train->speed > 0) {
            train->profile.velocity[train->speed] = velocity_old * 0.9 
                + agg_velocity * train->position.prev_arc->weight_factor * 0.1;
            train->velocity = train->profile.velocity[train->speed];
        }
        /*
        pprintf(COM2, "\033[%d;%dH\033[K[%d] v: (%d -> %d)\n\r", 
            24 + 19 + (line % 10), 1, Time(),
            (int)(velocity_old * 100), 
            (int)(train->profile.velocity[train->speed] * 100));

        pprintf(COM2, "\033[%d;%dH\033[K[%d] w: (%d -> %d)\n\r", 
            24 + 19 + (line % 10), 30, Time(),
            (int)(arc_weight_old * 100), 
            (int)(train->position.prev_arc->weight_factor * 100));

        pprintf(COM2, "\033[%d;%dH\033[K[%d] e: %d\n\r", 24 + 19 + (line % 10), 55,
            Time(), (int)train->position.estimated_next_sensor_dist);
        line++;
        */

        //pprintf(COM2, "\033[%d;%dH\033[Kdist: %d\n\r", 24 + 19, 1, train->position.prev_arc->dist);
        //pprintf(COM2, "\033[%d;%dH\033[Kvel: %d\n\r", 24 + 20, 1, (int)(agg_velocity * 100));
    }

    // TODO: calculate error
    train_model_update_location(train, time, switches);
    int err = (int)train->position.estimated_next_sensor_dist;

    pprintf(COM2, "\033[%d;%dH\033[Kerror:%d\n\r", 24 + 8, 1, err);

    train_model_init_location(train, time, switches, train->position.next_sensor);    
    //train->position.prev_sensor_dist = train->position.estimated_next_sensor_dist;

    train->position.prev_arc = train->position.arc;
    train->position.sensor_triggered_time = time;
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

inline void set_switch(int track_switch, char curve) {
    train_cmd((curve == STRAIGHT) ? SWITCHCODE | SMASK : SWITCHCODE | CMASK, 
              track_switch);
}

inline void set_switch_normalized(int track_switch, char curve) {
    //int tswitch = (track_switch > SWLENL)? track_switch - SWLENL + SWADDRBASEH - 1 : track_switch;
    set_switch((track_switch > SWLENL)? track_switch - SWLENL + SWADDRBASEH - 1 : track_switch, curve);
}

void track_set_switch(int location_server_tid, int track_switch, char curve, int soloff) {
    TERMmsg msg;
    msg.opcode = LOC_SWITCH_UPDATE;
    msg.param[0] = track_switch;
    msg.param[1] = curve;    

    Send(location_server_tid, &msg, sizeof(TERMmsg), 0, 0);

    set_switch_normalized(track_switch, curve);

    if (soloff) {
        Create(LOW_WORKER_PRIORITY, train_switch_task);
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
