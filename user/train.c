#include <train.h>
#include <io.h>
#include <syscall.h>
#include <priority.h>
#include <string.h>

#include <terminal_mvc_server.h>
#include <terminal_gui.h>
#include <train_location_server_task.h>

#include <assert.h>

//static int line = 0;

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
            // velocities for speeds 1-7 are basically made up so our acceleration
            // model doesn't screw up
            profile->velocity[1]  = 0.3;
            profile->velocity[2]  = 0.7;
            profile->velocity[3]  = 1.3;
            profile->velocity[4]  = 1.7;
            profile->velocity[5]  = 2.3;
            profile->velocity[6]  = 2.7;
            profile->velocity[7]  = 3.3;

            profile->velocity[8]  = 3.745661281;
            profile->velocity[9]  = 4.304676754;
            profile->velocity[10] = 4.774218154;
            profile->velocity[11] = 5.286317568;
            profile->velocity[12] = 5.79537037;
            profile->velocity[13] = 6.265265265;
            profile->velocity[14] = 6.303121853;

            // TODO: fix stopping distance
            profile->stop_distance[8] = 561.8;
            profile->stop_distance[9] = 643.6;
            profile->stop_distance[10] = 714.2;
            profile->stop_distance[11] = 797.4;
            profile->stop_distance[12] = 856.5;
            profile->stop_distance[13] = 962;
            profile->stop_distance[14] = 956;
    }
}

void train_calibration_work_init(TrainCalibrationWork* work) {
    work->num_arcs_passed = 0;
    work->dist_from_last_sensor = 0;
    work->weight_factors = 0;

    int i;
    for (i = 0; i < TRACK_MAX_EDGES_BTW_SENSORS; i++) {
        work->arcs_passed[i] = 0;
    }
}

void train_model_position_init(TrainModelPosition* position) {
    // useless until the position_known bit is turned on
    position->stop_node = (void *)0;
    position->stop_node_dist = -1;

    position->stop_dist = 0;
    position->stop_time = 0;
}

void train_model_init(TrainModel* train, int id) {
	train->id = id;
	train->speed = 0;
	train->previous_speed = 0;
    train->speed_updated_time = 0;
    train->velocity = 0;
    train->accel_const = 0;
    train->prev_accel = 0;

    train->bitmap = 0;
    train->bitmap |= TRAIN_MODEL_DIRECTION_FWD;

    train_calibration_profile_init(&train->profile, id);
    train_calibration_work_init(&train->calibration_work);
    train_model_position_init(&train->position);
}

// Train Model
void train_model_init_location(
    TrainModel* train, 
    int time, 
    int switches, 
    track_node* sensor_start) 
{
    train->bitmap |= TRAIN_MODEL_POSITION_KNOWN;

    // initialize first sensor node
    train->position.sensor_triggered_time = 0;

    train->position.dist_travelled = 0;

    ASSERT(sensor_start != (void *)0);

    train->position.arc = sensor_start->edge + DIR_AHEAD;

    train->position.next_sensor = track_next_sensor_node(switches, train->position.arc, &train->position.estimated_next_sensor_dist);
    // if train is going to an exit, stop tracking its location
    if (train->position.next_sensor == 0) {
        train->bitmap &= ~TRAIN_MODEL_POSITION_KNOWN;
    }

    train->position.updated_time = time;
    train_calibration_work_init(&train->calibration_work);
}

void train_model_update(TrainModel* train, int time, int switches) {
    TrainModelPosition* position = &train->position;
    TrainCalibrationProfile* profile = &train->profile;
    TrainCalibrationWork* work = &train->calibration_work;

    if (train->accel_const != 0) {
        // train is accelerating
        train->velocity = train->velocity + train->accel_const * TRAIN_ACCELERATION_DELTA;

        // turn off dynamic calibration if accelerating
        work->num_arcs_passed = 0;

        // TODO: I'm not sure how good floating point operations are
        if (train->accel_const * train->velocity >= train->accel_const * profile->velocity[train->speed]) {
            // turn off acceleration if reached target velocity
            train->prev_accel = train->accel_const;
            train->accel_const = 0;
            train->velocity = profile->velocity[train->speed];
            //debugf("[%d] (done acceleration)", Time());
        }

        // update stopping distance and stopping time
        float stop_time = train->velocity / TRAIN_ACCELERATION;
        float stop_dist = train->velocity * stop_time - 0.5 * TRAIN_ACCELERATION * stop_time * stop_time;

        position->stop_time = (int) stop_time;
        position->stop_dist = (int) stop_dist;
        ASSERTP(position->stop_time >= 0, "negative stop time %d", (int)(stop_time * 100));
        ASSERTP(position->stop_dist >= 0, "negative stop dist %d", (int)(stop_dist * 100));

        /*
        debugf("[%d] (dist %d, time %d)", 
                Time(), position->stop_dist, position->stop_time);*/

        if (train->accel_const == 0) {
            // update stopping distance in profile
            profile->stop_distance[train->speed] = stop_dist;
        }
    }
    train_model_update_location(train, time, switches);
}

void train_model_update_location(TrainModel* train, int time, int switches) {
    TrainModelPosition* position = &train->position;
    //TrainCalibrationProfile* profile = &train->profile;
    TrainCalibrationWork* work = &train->calibration_work;

    if (train->bitmap & TRAIN_MODEL_POSITION_KNOWN) {
        ASSERT(position->arc != (void *)0);
        ASSERT(position->next_sensor != (void *)0);
        float vel_multiplier = 1.0 / position->arc->weight_factor;
        float delta_dist = (time - position->updated_time) * train->velocity * vel_multiplier;

        // if train is lost
        position->estimated_next_sensor_dist -= delta_dist;
        if (position->estimated_next_sensor_dist < TRAIN_SENSOR_HIT_TOLERANCE) {
            train->bitmap &= ~TRAIN_MODEL_POSITION_KNOWN;
            debugf("[%d] (train lost, expected %s)", 
                    Time(), position->next_sensor->name);
        }

        position->dist_travelled += delta_dist;

        position->arc = track_next_arc(switches, position->arc, &position->dist_travelled);

        // keep track of the arcs a train passed by between two sensors
        if (position->arc != (void *)0
            && work->num_arcs_passed > 0      // the train passed by the first arc's sensor
            && position->arc != work->arcs_passed[work->num_arcs_passed - 1] // this is a new arc
            && position->estimated_next_sensor_dist > 0) {    // the train has not passed the second sensor

            ASSERT(position->arc != (void *)0);
            ASSERT(work->arcs_passed[0]->src->type == NODE_SENSOR);
            ASSERTP(work->num_arcs_passed < TRACK_MAX_EDGES_BTW_SENSORS, 
                    "%d num_arcs_passed reached limit, passed another arc", work->num_arcs_passed);

            work->arcs_passed[work->num_arcs_passed] = position->arc;
            work->num_arcs_passed = work->num_arcs_passed + 1;

            work->dist_from_last_sensor += position->arc->dist;
            work->weight_factors += position->arc->dist * position->arc->weight_factor;
        }

        if (position->arc == (void *)0)
            train->bitmap &= ~TRAIN_MODEL_POSITION_KNOWN;

        position->updated_time = time;
    }
}

track_edge* track_next_arc(int switches, track_edge* current, float* dist) {
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
                current = node->edge + get_bit(switches, switch_num);
                break;
            }
            case NODE_EXIT:
            default:
                return (void *)0;
        }
    }
    return current;
}

track_node* track_next_sensor_node(int switches, track_edge* current, float* dist) {
    *dist = current->dist;
    track_node* node = current->dest;
    while (node->type != NODE_SENSOR) {
        switch (node->type) {
            case NODE_BRANCH: {
                int switch_num = node->num;
                *dist += node->edge[get_bit(switches, switch_num)].dist;
                node = node->edge[get_bit(switches, switch_num)].dest;
            break;
            }
            case NODE_MERGE:
            case NODE_ENTER:
                *dist += node->edge[DIR_AHEAD].dist;
                node = node->edge[DIR_AHEAD].dest;
            break;
            case NODE_EXIT:
            default:
                return (void *)0;
        }
    }
    return node;
}

int track_ahead_contain_node(
    track_node* node, int switches, track_node* current, 
    int lookahead_dist, int *dist_to_node) {

    int total_arc_dist = 0;

    //int time = Time();
    int arc_travelled = 0;
    while (node != current) {
        //debugf("[%d]search %s to %s.", time, current->name, node->name);
        track_edge* next_arc = (void *)0;
        switch (current->type) {
            case NODE_MERGE:
            case NODE_SENSOR:
            case NODE_ENTER:
                next_arc = current->edge + DIR_AHEAD;
                break;
            case NODE_BRANCH: {
                //debugf("[%d] switches[%d] = %d.", time, node->num, switches[node->num]);
                next_arc = current->edge + get_bit(switches, current->num);
                break;
            }
            case NODE_EXIT:
            default: // can't search no more
                return 0;
        }

        lookahead_dist = lookahead_dist - next_arc->dist;
        total_arc_dist = total_arc_dist + next_arc->dist;

        ASSERT(total_arc_dist > 0);

        if (lookahead_dist < 0)
            break;
        arc_travelled ++;
        current = next_arc->dest;
    }

    /*
    debugf("arc_travelled: %d.", arc_travelled);
    if (node == current && total_arc_dist == 0) {
        debugf("found total arc_dist: %d", total_arc_dist);
        //Delay(30);
        //ASSERT(total_arc_dist > 0);
    }
    */

    *dist_to_node = total_arc_dist;
    return node == current;   
}

void train_model_update_speed(TrainModel* train, int time, int switches, int speed) {
    train_model_update_location(train, time, switches);
    
    if (speed != train->speed) {
        train->previous_speed = train->speed;
        train->speed = speed;
        train->speed_updated_time = time;
    
        train->prev_accel = train->accel_const;
        train->accel_const = (train->previous_speed < train->speed)? 1 : -1;
        //debugf("[%d] (start acceleration)", Time());
    }
}

void train_model_reverse_direction(TrainModel* train, int time, int switches) {
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
void train_model_next_sensor_triggered(TrainModel* train, int time, int switches, int* error) {
    TrainModelPosition* position = &train->position;
    TrainCalibrationProfile* profile = &train->profile;
    TrainCalibrationWork* work = &train->calibration_work;

    ASSERTP(work->num_arcs_passed <= TRACK_MAX_EDGES_BTW_SENSORS, 
            "num_arcs_passed exceeded limit, got %d", 
            work->num_arcs_passed);

    // dynamically calibrate velocity and track
    // We want to wait for the train to accelerate enough after a change of speed before calibration
    if (train->accel_const == 0 && work->num_arcs_passed > 0) {
        int i;

        int time_delta = time - position->sensor_triggered_time;    // delta time
        int agg_distance = work->dist_from_last_sensor;         // total distance between sensors
        ASSERT(agg_distance > 0);
        float agg_weight_factor = work->weight_factors / agg_distance;    // weighted average of track weight factor between sensors
        /*
        for (i = 0; i < position->num_arcs_passed; i++) {
            track_edge* edge = position->arcs_passed[i];
            agg_distance += edge->dist;
            agg_weight_factor += edge->dist * edge->weight_factor; 
        }*/

        //int distance = train->position.prev_arc->dist;
        float agg_velocity = (float) agg_distance / time_delta;

        // for debug prints
        //float velocity_old = profile->velocity[train->speed];

        if (train->speed > 0) {
            profile->velocity[train->speed] = profile->velocity[train->speed] * 0.9 
                + agg_velocity * agg_weight_factor * 0.1;
            train->velocity = profile->velocity[train->speed];
        }

        // FIXME: Updating the weight factors relative to the actual edge lengths is too hard
        // Instead, update each weight with the average weight
        for (i = 0; i < work->num_arcs_passed; i++) {
            track_edge* arc = work->arcs_passed[i];
            //float arc_weight_old = arc->weight_factor;

            arc->weight_factor = arc->reverse->weight_factor = arc->weight_factor * 0.9 
                               + profile->velocity[train->speed] * time_delta / agg_distance * 0.1;

            /*
            debugf("[%d] v: (%d -> %d)\n\r", 
                Time(),
                (int)(arc_weight_old * 100), 
                (int)(arc->weight_factor * 100));*/
        }
    }

    // calculate error
    train_model_update_location(train, time, switches);
    *error = (int)position->estimated_next_sensor_dist;

    //debugf("error:%d\n\r", *error);

    train_model_init_location(train, time, switches, position->next_sensor);    
    //train->position.prev_sensor_dist = train->position.estimated_next_sensor_dist;

    //train->position.prev_arc = train->position.arc;
    // initialize variables for next round of dynamic calibration
    work->arcs_passed[0] = position->arc;

    work->num_arcs_passed = 1;
    work->dist_from_last_sensor = position->arc->dist;
    work->weight_factors = position->arc->dist * position->arc->weight_factor;
    position->sensor_triggered_time = time;
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
