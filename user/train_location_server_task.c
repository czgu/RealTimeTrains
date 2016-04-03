#include <terminal_mvc_server.h>
#include <train_logic_task.h>

#include <train_location_server_task.h>
#include <courier_warehouse_task.h>

#include <syscall.h>
#include <io.h>

#include <train.h>
#include <track_data.h>
#include <rqueue.h>
#include <priority.h>

#include <assert.h>

void train_location_server_secretary_task() {
    RegisterAs("Location Server");

    int sender;
    TERMmsg request_msg;
    
    TERMmsg request_msg_pool[40];
    RQueue request_msg_buffer;
    rq_init(&request_msg_buffer, request_msg_pool, 40, sizeof(TERMmsg));

    int location_server = MyParentTid();
    int courier = Create(TRAIN_LOCATION_SERVER_COURIER_PRIORITY, courier_task);
    int courier_ready = 0;

    Send(courier, &location_server, sizeof(int), 0, 0);    

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case COURIER_NOTIF:
                    courier_ready = 1;
                    break;
                case LOC_TRAIN_SPEED_UPDATE:
                case LOC_TRAIN_SPEED_REVERSE_UPDATE:
                case LOC_SWITCH_UPDATE:
                case LOC_SENSOR_MODULE_UPDATE:
                case LOC_SET_TRAIN_DEST:
                    Reply(sender, 0, 0);
                    rq_push_back(&request_msg_buffer, &request_msg);
                    break;
                case LOC_WAIT_SENSOR:
                case LOC_WHERE_IS:
                case LOC_QUERY:
                    request_msg.extra = sender;
                    rq_push_back(&request_msg_buffer, &request_msg);
                    break;
            }
        }

        if (courier_ready && !rq_empty(&request_msg_buffer)) {
            request_msg = *((TERMmsg *)rq_pop_front(&request_msg_buffer));        
            Reply(courier, &request_msg, sizeof(TERMmsg));

            courier_ready = 0;
        }
    }
}

void train_location_server_task() {
    Create(TRAIN_LOCATION_SERVER_SECRETARY_PRIORITY, 
           train_location_server_secretary_task);
    Create(TRAIN_LOCATION_TICKER_PRIORITY, train_location_ticker);

    // init train
    int i, t;
    int num_train = TRAIN_ID_MAX - TRAIN_ID_MIN + 1;
    TrainModel train_models[num_train];
    for (i = 0; i < num_train; i++) {
        train_model_init(train_models + i, i + TRAIN_ID_MIN);
    }
    TrainModel* active_train_models[num_train];
    int num_active_train = 0;

    // init switch
    // FIXME: why is this a short
    short switches[NUM_TRAIN_SWITCH + 1];
    for (i = 0; i < NUM_TRAIN_SWITCH + 1; i++) {
        switches[i] = DIR_STRAIGHT;
    }

    // init sensor
    unsigned short sensor_bitmap[SNLEN] = {0};

    WaitModule wait_modules[SNLEN];
    for (i = 0; i < SNLEN; i++) {
        //t = i >= 19 ? i - 19 + 153 : i;
        wait_module_init(wait_modules + i);

    }

    // TODO: add option or something to init track a
    // init track
    init_trackb(train_track);

    int sender;
    TERMmsg request_msg;

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            // Only courier and timer tasks send to this
            Reply(sender, 0, 0);
            switch(request_msg.opcode) {
                case LOC_WAIT_SENSOR:
                    wait_module_add(wait_modules + request_msg.param[0], request_msg.param[1], request_msg.extra);
                    break;
                case LOC_TRAIN_SPEED_UPDATE: {
                    int train_index = request_msg.param[0] - TRAIN_ID_MIN;
                    int speed = request_msg.param[1];
                
                    if (train_index >= 0 && train_index < num_train && speed >= 0 && speed <= 14) 
                    {
                        if (speed > 0 && !(train_models[train_index].bitmap & TRAIN_MODEL_ACTIVE)) {
                            active_train_models[num_active_train++] = train_models + train_index;

                            train_models[train_index].bitmap |= TRAIN_MODEL_ACTIVE;

                            // Create a tracer program 
                            int train_id = train_models[train_index].id;
                            int cid = Create(15, train_tracer_task);
                            Send(cid, &train_id, sizeof(int), 0, 0);
                        }
                        // set speed in the model
                        train_model_update_speed(train_models + train_index, Time(), switches, speed);
                    }
                    break;
                }
                case LOC_TRAIN_SPEED_REVERSE_UPDATE: {
                    int train_index = request_msg.param[0] - TRAIN_ID_MIN;

                    if (train_index >= 0 && train_index < num_train) {
                        //reverse the direction in the model
                        train_model_reverse_direction(train_models + request_msg.param[0] - TRAIN_ID_MIN, Time(), switches);                
                    }
                    break;
                }
                case LOC_SWITCH_UPDATE: {
                    int switch_index = request_msg.param[0];
                    if (switch_index >= 153) {
                        switch_index -= 134; 
                    }

                    if (switch_index >= 0 && switch_index <= NUM_TRAIN_SWITCH) {
                        // update switch condition
                        switches[switch_index] = request_msg.param[1];
                    }
                    break;
                }
                case LOC_SENSOR_MODULE_UPDATE: {
                    // TODO: this has a bad worse case time
                    int module = request_msg.param[0];
                    //unsigned short old_bitmap = sensor_bitmap[module];
                    unsigned short new_bitmap = request_msg.param[1] << 8 | request_msg.param[2];
                    //unsigned short delta_bitmap = 0;

                    sensor_bitmap[module] = new_bitmap;

                    if (new_bitmap != 0) {
                        wait_module_update(wait_modules + module, new_bitmap);
                        
                        int time = Time();
                        TrainModel* lost_train = (void *)0;

                        for (t = 0; t < num_active_train; t++) {
                            TrainModel* train = active_train_models[t];

                            if (train->bitmap & TRAIN_MODEL_POSITION_KNOWN) {
                                int sensor_idx = train->position.next_sensor->num;
                                if (module == sensor_idx / 16 && 
                                    (new_bitmap & (0x8000 >> (sensor_idx % 16)))) {
                                    train_model_next_sensor_triggered(train, time, switches);

                                    // Sensor gets 'consumed'
                                    new_bitmap &= ~(0x8000 >> (sensor_idx % 16));
                                }
                            } else if (train->speed > 0) {
                                lost_train = train;
                            }
                        }

                        // This is for one lost train
                        if (lost_train != (void *)0 && new_bitmap != 0) {
                            int sensor = -1;

                            // we need 1 sensor, and we should get exact 1 sensor?
                            for (i = 0; i < 16; i++) {
                                if (new_bitmap & (0x8000 >> i)) {
                                    sensor = i;
                                    break;
                                }
                            }

                            if (sensor >= 0) {
                                lost_train->bitmap |= TRAIN_MODEL_TRACKED;
                                train_model_init_location(lost_train, time, switches, train_track + (module * 16 + sensor));
                            }
                        }
                    }

                    break;
                }
                case LOC_WHERE_IS: {
                    int train = request_msg.param[0] - TRAIN_ID_MIN;   

                    if (train >= 0 && train_models[train].bitmap & TRAIN_MODEL_TRACKED) {
                        Reply(request_msg.extra, &train_models[train].position, sizeof(TrainModelPosition));
                    } else {
                        Reply(request_msg.extra, 0, 0);
                    }
                    break;
                }
                case LOC_TIMER_UPDATE: {
                    int time = Time();
                    
                    for (t = 0; t < num_active_train; t++) {
                        TrainModel* train = active_train_models[t];

                        if (train->bitmap & TRAIN_MODEL_POSITION_KNOWN) {
                            train_model_update_location(train, time, switches);

                            if (train->position.stop_node != (void *)0 && train->speed > 0) {
                                //ASSERTP((((int)train->profile.stop_distance[train->speed]) > 0), "train %d speed %d", train->id, train->speed);
                                int lookahead = ((int)train->profile.stop_distance[train->speed]) + TRAIN_LOOK_AHEAD_DIST + train->position.dist_travelled + train->position.stop_dist;
                                int dist_to_node;
                                if (track_ahead_contain_node(
                                        train->position.stop_node, 
                                        switches, 
                                        train->position.arc->src, 
                                        lookahead, 
                                        &dist_to_node))
                                {
                                    float dist_left = (float)dist_to_node
                                        + train->position.stop_dist 
                                        - train->position.dist_travelled 
                                        - train->profile.stop_distance[train->speed];

                                    // ASSERT(dist_to_node > 0);
                                    int instruction[2];
                                    // TODO: might change this to train->velocity
                                    instruction[0] = ((int)(dist_left * train->position.arc->weight_factor / train->profile.velocity[train->speed]));
                                    instruction[1] = train->id;
                                    //ASSERTP(dist_left > 0, " dist_to_node %d, arc_dist %d, dist_travelled %d, stop_dist %d, lookahead %d", (int)dist_to_node, (int)train->position.arc->dist, (int)train->position.dist_travelled, (int)train->profile.stop_distance[train->speed], lookahead);

                                    pprintf(COM2, "\033[%d;%dH", 44, 1);
                                    pprintf(COM2, "[%d] found %d %d %d %d %d.", Time(), instruction[0], (int)dist_to_node, (int)dist_left, (int)lookahead, (int)(train->position.arc->dist - train->position.dist_travelled));
                                    int cid = Create(10, train_timed_stop_task);
                                    Send(cid, instruction, sizeof(int) * 2, 0, 0);
                                    train->position.stop_node = (void *)0;
                                    train->position.stop_dist = -1;
                                }


                            } 
                        }
                    }
                    break;
                }
                case LOC_SET_TRAIN_DEST: {
                    int train = request_msg.param[0] - TRAIN_ID_MIN;
                    int node = (int)request_msg.param[1];
                    int dist = (request_msg.param[2] << 8 | request_msg.param[3]);

                    train_models[train].position.stop_node = train_track + node;
                    train_models[train].position.stop_dist = dist;

                    pprintf(COM2, "\033[%d;%dH", 43, 1);
                    pprintf(COM2, "got %d, %d, %d.", train, node ,dist);
                    break;
                }
                case LOC_QUERY: {
                    int reply = 0;
                    switch(request_msg.param[0]) {
                        case 0: // switch
                            reply = sensor_bitmap[(int)request_msg.param[1]];
                            break;
                        case 1: // train speed
                            reply = train_models[(int)request_msg.param[1] - TRAIN_ID_MIN].speed;
                            break;
                        case 2: // stop dist
                            reply = train_models[(int)request_msg.param[1] - TRAIN_ID_MIN].position.stop_dist;
                            break;
                    }
                    Reply(sender, &reply, sizeof(int));
                }
            }
            /*
            if (request_msg.opcode != LOC_SENSOR_MODULE_UPDATE) {
                pprintf(COM2, "\033[%d;%dH", 24 + i++ % 15, 1);
                pprintf(COM2, "location: %d,%d,%d", request_msg.opcode, request_msg.param[0], request_msg.param[1]);
            }
            */
        }
    }

}

void train_tracer_task() {
    int sender, train_id;
    Receive(&sender, &train_id, sizeof(int));
    Reply(sender, 0, 0);
    //msg.param[0] contains the train #

    // weird sychronization bug which cause it to be indefinitely blocked
    int view_server = WhoIs("View Server");
    int location_server = WhoIs("Location Server");

    TrainModelPosition position;

    TERMmsg draw_msg;
    draw_msg.opcode = DRAW_TRAIN_LOC;
    draw_msg.param[0] = train_id;

    int known_position;

    for (;;) {
        known_position = where_is(location_server, train_id, &position);

        // FIXME: seems like if we update too fast, we see weird distance values
        if (known_position) {
            draw_msg.param[1] = position.arc->src->type;
            draw_msg.param[2] = position.arc->src->num;

            draw_msg.param[3] = position.next_sensor->type;
            draw_msg.param[4] = position.next_sensor->num;

            //draw_msg.param[3] = position.arc->dest->type;
            //draw_msg.param[4] = position.arc->dest->num;

            short dist = (short)position.dist_travelled;

            draw_msg.param[5] = dist >> 8;
            draw_msg.param[6] = dist & 0xFF;

            Send(view_server, &draw_msg, sizeof(TERMmsg), 0, 0);
        }
        
        /*
        pprintf(COM2, "\033[%d;%dH", 25, 1);

        if (sz > 0) {
            pprintf(COM2, "%s %d", position.arc->src->name, (int)position.dist_travelled);
        } else {
            pprintf(COM2, "[%d]", Time());
            PutStr(COM2, "Unknown position");
        }        
        */
        // TODO: might want to change this later
        Delay(100);
    }
}

void train_sensor_task() {
    int view_server = WhoIs("View Server");
    int location_server = WhoIs("Location Server");
    TERMmsg msg;

    SensorData sensors[SNLEN] = {{0}};  // only sensors [0, SNLEN - 1] are valid
                                        // which is A = 0,B,C,D,E
    int i;
    for (;;) {
        sensor_request_upto(SNLEN);

        for (i = 0; i < SNLEN; i++) {
            char lo = Getc(COM1);
            char hi = Getc(COM1);

            char dlo = (lo ^ sensors[i].lo) & ~sensors[i].lo;
            char dhi = (hi ^ sensors[i].hi) & ~sensors[i].hi;

            // send updates to the location server only if a sensor is triggered
            // we only care all 0->1 changes on screen
            if (dhi != 0 || dlo != 0) {
                msg.opcode = LOC_SENSOR_MODULE_UPDATE;
                msg.param[0] = i;
                msg.param[1] = dlo;
                msg.param[2] = dhi;
                Send(location_server, &msg, sizeof(TERMmsg), 0, 0);

                msg.opcode = DRAW_MODULE;
                msg.param[0] = i;
                msg.param[1] = dlo;
                msg.param[2] = dhi;
                Send(view_server, &msg, sizeof(TERMmsg), 0, 0);
            }

            sensors[i].lo = lo;
            sensors[i].hi = hi;
        }
    }
}

void train_location_ticker() {
    int pid = MyParentTid();
    TERMmsg msg;
    msg.opcode = LOC_TIMER_UPDATE;
    
    for (;;) {
        Send(pid, &msg, sizeof(char), 0, 0);
        Delay(TRAIN_LOCATION_UPDATE_TIME_INTERVAL);
    }
}

void train_timed_stop_task() {
    int instruction[2];
    int sender;
    Receive(&sender, instruction, sizeof(int) * 2);
    Reply(sender, 0, 0);
    
    int location_server = WhoIs("Location Server");

    if (instruction[0] > 0)
        Delay(instruction[0]);
    
    train_set_speed(location_server, instruction[1], 0);
}

int where_is(int location_server_tid, int train_id, TrainModelPosition* position) {
    TERMmsg msg;
    msg.opcode = LOC_WHERE_IS;
    msg.param[0] = train_id;

    int sz = Send(location_server_tid, &msg, sizeof(TERMmsg), position, sizeof(TrainModelPosition));
    
    return sz;
}

void wait_sensor(int location_server_tid, char module, int sensor) {
    TERMmsg msg;
    msg.opcode = LOC_WAIT_SENSOR;
    msg.param[0] = module - 1;
    msg.param[1] = sensor - 1;

    Send(location_server_tid, &msg, sizeof(TERMmsg), 0, 0);
}

void stop_train_at(int location_server_tid, int train_id, int node_id, int dist) {
    TERMmsg msg;
    msg.opcode = LOC_SET_TRAIN_DEST;
    msg.param[0] = train_id;
    msg.param[1] = node_id;
    msg.param[2] = dist >> 8;
    msg.param[3] = dist & 0xFF;

    Send(location_server_tid, &msg, sizeof(TERMmsg), 0, 0);
}

void wait_module_init(WaitModule* wm) {
    int i;
    for (i = 0; i < 16; i++)
        wm->sensor_tid[i] = -1;
    wm->wait_num = 0;
}

inline void wait_module_add(WaitModule* wm, char sensor, int tid) {
    if (wm->sensor_tid[(int)sensor] >= 0) {
        // TODO: do something to kick out the other waiting task?
    } else {
        wm->wait_num++;
    }
    wm->sensor_tid[(int)sensor] = tid;
}

inline void wait_module_update(WaitModule* wm, unsigned short bitmap) {
    if (wm->wait_num == 0 || bitmap == 0)
        return;

    int i;
    for (i = 0; i < 16; i++) {
        if (wm->sensor_tid[i] >= 0 && (bitmap & (0x8000 >> i))) {
            Reply(wm->sensor_tid[i], 0, 0);
            wm->sensor_tid[i] = -1;
            wm->wait_num --;
        }
    }

}

int location_query(int location_server_tid, int query_type, int query_val) {
    TERMmsg msg;
    msg.opcode = LOC_QUERY;
    msg.param[0] = query_type;
    msg.param[1] = query_val;

    int ret = 0;

    Send(location_server_tid, &msg, sizeof(TERMmsg), &ret, sizeof(int));

    return ret;
}
