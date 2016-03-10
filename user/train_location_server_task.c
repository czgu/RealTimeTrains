#include <terminal_mvc_server.h>
#include <train_logic_task.h>

#include <train_location_server_task.h>
#include <courier_warehouse_task.h>

#include <syscall.h>
#include <io.h>

#include <train.h>
#include <track_data.h>
#include <rqueue.h>

void train_location_server_secretary_task() {
    RegisterAs("Location Server");

    int sender;
    TERMmsg request_msg;
    
    TERMmsg request_msg_pool[20];
    RQueue request_msg_buffer;
    rq_init(&request_msg_buffer, request_msg_pool, 20, sizeof(TERMmsg));

    int location_server = MyParentTid();
    int courier = Create(10, courier_task);
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
                    Reply(sender, 0, 0);
                    rq_push_back(&request_msg_buffer, &request_msg);
                    break;
                case LOC_WAIT_SENSOR:
                case LOC_WHERE_IS:
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
    Create(10, train_location_server_secretary_task);


    // init train
    int i;
    int num_train = TRAIN_ID_MAX - TRAIN_ID_MIN + 1;
    TrainModel train_models[num_train];
    for (i = 0; i < num_train; i++) {
        train_model_init(train_models + i, i + TRAIN_ID_MIN);
    }

    // init switch
    short switches[NUM_TRAIN_SWITCH + 1] = {DIR_STRAIGHT};

    // init sensor
    unsigned short sensor_bitmap[SNLEN] = {0};

    WaitModule wait_modules[SNLEN];
    for (i = 0; i < SNLEN; i++)
        wait_module_init(wait_modules + i);

    // init track
    init_trackb(train_track);

    int sender;
    TERMmsg request_msg;

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            // Only courier send to this
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
                    
                        int t;
                        for (t = 0; t < TRAIN_ID_MAX - TRAIN_ID_MIN + 1; t++) {
                            if (train_models[t].position_known) {
                                int sensor_idx = train_models[t].position.next_sensor->num;
                                if (module == sensor_idx / 16 && 
                                    (new_bitmap & (0x8000 >> (sensor_idx % 16)))) {
                                    train_model_next_sensor_triggered(train_models + t, time, switches);
                                }
                            } else if(train_models[t].speed > 0) {
                                int sensor = -1;

                                // we need 1 sensor, and we should get exact 1 sensor
                                
                                for (i = 0; i < 16; i++) {
                                    if (new_bitmap & (0x8000 >> i)) {
                                        if (sensor == -1) {
                                            sensor = i;
                                        }
                                        else {
                                            sensor = -1;
                                            break;
                                        }
                                    }
                                }

                                if (sensor >= 0) {
                                    train_model_init_location(train_models + t, time, switches, train_track + (module * 16 + sensor));
                                    // Create a tracer program 
                                    int train_id = t + TRAIN_ID_MIN;
                                    int cid = Create(15, train_tracer_task);
                                    Send(cid, &train_id, sizeof(int), 0, 0);
                                }
                            
                            }
                        }
                    }

                    break;
                }
                case LOC_WHERE_IS: {
                    int train = request_msg.param[0] - TRAIN_ID_MIN;   

                    if (train_models[train].position_known) {
                        train_model_update_location(train_models + train, Time(), switches);
                        Reply(request_msg.extra, &train_models[train].position, sizeof(TrainModelPosition));
                    } else {
                        Reply(request_msg.extra, 0, 0);
                    }
                    break;
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
    int view_server = WhoIs("View Server");
    int location_server = WhoIs("Location Server");

    int sender, train_id;
    Receive(&sender, &train_id, sizeof(int));
    Reply(sender, 0, 0);
    //msg.param[0] contains the train #

    TERMmsg msg;
    msg.opcode = LOC_WHERE_IS;
    msg.param[0] = train_id;

    TrainModelPosition position;

    TERMmsg draw_msg;
    draw_msg.opcode = DRAW_TRAIN_LOC;
    draw_msg.param[0] = train_id;

    int sz;

    for (;;) {
        sz = Send(location_server, &msg, sizeof(TERMmsg), &position, sizeof(TrainModelPosition));

        if (sz > 0) {
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
            if (lo != 0 || hi != 0) {
                msg.opcode = LOC_SENSOR_MODULE_UPDATE;
                msg.param[0] = i;
                msg.param[1] = lo;
                msg.param[2] = hi;
                Send(location_server, &msg, sizeof(TERMmsg), 0, 0);
            }

            // we only care all 0->1 changes on screen
            if (dhi != 0 || dlo != 0) {
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

void wait_sensor(int location_server_tid, char module, int sensor) {
    TERMmsg msg;
    msg.opcode = LOC_WAIT_SENSOR;
    msg.param[0] = module - 1;
    msg.param[1] = sensor - 1;

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
