#include <train_route_server.h>
#include <syscall.h>

#include <train_location_server_task.h>
#include <terminal_mvc_server.h>

#include <io.h>

#include <train.h>
#include <string.h>
#include <dijkstra.h>

int execute_route(Route* route, int train_id, int location_server, int route_server, TrainModelPosition* position);

static int line = 0;

void train_route_server() {
    RegisterAs("Route Server");

    int sender;
    TERMmsg request_msg;

    // reserve nodes
    char reserved_nodes[TRACK_MAX];
    memset(reserved_nodes, 0, TRACK_MAX*sizeof(char));

    // reservation bitmap
    int reservation_bitmap[TRACK_BITMAP_MAX];
    memset(reservation_bitmap, 0, TRACK_BITMAP_MAX*sizeof(int));

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case RESERVE_NODE: {
                    int success = 0;

                    int node_id = (int)request_msg.param[0];
                    int train_id = (int)request_msg.param[1];

                    if (reserved_nodes[node_id] == 0) {
                        reserved_nodes[node_id] = train_id;

                        track_node* reverse_node = train_track[node_id].reverse;
                        if (reverse_node != (void *)0) {
                            reservation_bitmap[reverse_node->id/sizeof(int)] |= (0x1 << (reverse_node->id % 32));
                            reserved_nodes[reverse_node->id] = train_id;
                        }
                    

                        success = 1;
                    } else if (reserved_nodes[node_id] == train_id) {
                        success = 1;
                    } else {
                        pprintf(COM2, "\033[%d;%dH\033[K Alloc: %d train: %d, owner: %d, status:%d", 25 + line ++ % 10, 1,  node_id, train_id, reserved_nodes[node_id], success);
                    }

                    reservation_bitmap[node_id/sizeof(int)] |= (0x1 << (node_id % 32));
                    Reply(sender, &success, sizeof(int));
                    break;
                }
                case RELEASE_NODE: {
                    Reply(sender, 0, 0);

                    int node_id = (int)request_msg.param[0];
                    int train_id = (int)request_msg.param[1];

                    if (reserved_nodes[node_id] == train_id) {
                        reserved_nodes[node_id] = 0;
                        reservation_bitmap[node_id/sizeof(int)] &= ~(0x1 << (node_id % 32));

                        track_node* reverse_node = train_track[node_id].reverse;
                        if (reverse_node != (void *)0) {
                            reservation_bitmap[reverse_node->id/sizeof(int)] &= ~(0x1 << (reverse_node->id % 32));
                            reserved_nodes[reverse_node->id] = 0;
                        }
                    }

                    pprintf(COM2, "\033[%d;%dH\033[K Free: %d train: %d", 25 + line ++ % 10, 1,  request_msg.param[0], request_msg.param[1]);
                    break;
                }
                case RELEASE_ALL: {
                    Reply(sender, 0, 0);
                    int i;

                    int train_id = (int)request_msg.param[0];
                    int node_id = (int)request_msg.param[1];

                    for (i = 0; i < TRACK_MAX; i++) {
                        if (reserved_nodes[i] == train_id) {
                            reservation_bitmap[i] &= ~(0x1 << (i % 32));
                            reserved_nodes[i] = 0;
                        }
                    }

                    reserved_nodes[node_id] = train_id;
                    track_node* reverse_node = train_track[node_id].reverse;
                    if (reverse_node != (void *)0) {
                        reservation_bitmap[reverse_node->id/sizeof(int)] |= (0x1 << (reverse_node->id % 32));
                        reserved_nodes[reverse_node->id] = train_id;
                    }

                    break;
                }
                case GET_RESERVE_DATA: {
                    Reply(sender, reservation_bitmap, TRACK_BITMAP_MAX * sizeof(int));
                    break;
                }
            }
        }
    }
}

void train_route_worker() {
    char instruction[2];
    int sender;

    Receive(&sender, instruction, sizeof(char) * 2);
    Reply(sender, 0, 0);

    int time = Time();

    pprintf(COM2, "\033[%d;%dH\033[K[%d]Route start",35 + line++ % 10, 1, time);

    int track_taken[TRACK_BITMAP_MAX];
    memset(track_taken, 0, TRACK_BITMAP_MAX*sizeof(int));    

    int train_id = instruction[0];
    int dest_idx = instruction[1];

    TrainModelPosition position;

    int location_server = WhoIs("Location Server");
    int route_server = WhoIs("Route Server");

    train_set_speed(location_server, train_id, 0);
    Delay(300);

    if (where_is(location_server, train_id, &position) <= 0) {
        pprintf(COM2, "\033[%d;%dH\033[KCan't find train %d.", 35 + line++ % 10, 1, train_id);
        return;
    }

    release_all_track(route_server, train_id, position.arc->dest->id);

    Path path; // path in graph theory model
    Route route; // route for actual execution

    // 0: good, -1 : bad
    int path_status = -1;

    int tries = 0;

    while (path_status < 0) {
        //get_track_reservation(route_server, track_taken);
        // turn off destination bit
        track_taken[position.arc->dest->id/sizeof(int)] &= ~(0x1 << (position.arc->dest->id % 32));


        dijkstra_find(position.arc->src, train_track + dest_idx, &path, track_taken);
        path_to_route(&path, &route);

        pprintf(COM2, "\033[%d;%dH\033[K Path [%d] : ", 33, 1, train_id);
        int i;
        for (i = 0; i < route.route_len; i++) {
            pprintf(COM2, "%s (%d) -> ", route.nodes[i].node->name, route.nodes[i].action );
        }
        PutStr(COM2, " END\n\r");

        if (route.route_len <= 1) {
            pprintf(COM2, "\033[%d;%dH\033[K[%d] No path. ", 35, 1, time);
            break;
        } else {
            path_status = execute_route(&route, train_id, location_server, route_server, &position);

            // Release the root except the current node
            Delay(300);
            tries ++;

            where_is(location_server, train_id, &position);
            release_all_track(route_server, train_id, position.arc->dest->id);
        }

        if (tries > 5) {
            pprintf(COM2, "\033[%d;%dH\033[K[%d]Route find failed", 35 + line++ % 10, 1, time);
            break;
        }
    }

    pprintf(COM2, "\033[%d;%dH\033[K[%d]Route end", 35 + line++ % 10, 1, time);
}

int execute_route(Route* route, int train_id, int location_server, int route_server, TrainModelPosition* position) {
    //int time = Time();
    int current_route = 0;
    int route_len = route->route_len;
    //int i; 
    int lookahead = 800;
    int lookbehind = 300;
    int status = 0;

    train_set_speed(location_server, train_id, 8);
    for (;;) {
        if (where_is(location_server, train_id, position) <= 0) {
            pprintf(COM2, "\033[%d;%dH\033[K Cant find train %d", 35 + line++ % 10, 1, train_id);
            Delay(25);
            continue;
        }

        for (current_route = (current_route < 5 ? 0 : (current_route - 5)); current_route < route_len; current_route++) {
            if (route->nodes[current_route].node == position->arc->src)
                break;
        }

        if (current_route >= route_len) {
            pprintf(COM2, "\033[%d;%dH\033[KCan't find node %s.", 35 + line++ % 10, 1, position->arc->src->name);
            return -1; 
        } else if (current_route >= route_len - 2) {
            //train_set_speed(location_server, train_id, 0);

            // if the train is not moving
            if (location_query(location_server, 1, train_id) == 0) {
                break;
            }
        }

        // release track
        lookbehind_node(route, current_route, lookbehind, route_server, train_id);
        
        // reserve track and do action
        status = lookahead_node(route, current_route, lookahead, location_server, route_server, train_id);
        

        if (status != 0) {
            if (status == -1) {
                return -1;
            }

            if(status == 1) {
                pprintf(COM2, "\033[%d;%dH\033[KExecute_route done.\n\r", 35 + line++ % 10, 1);
                return 0;
            }
        }

        Delay(5);
    }
    return 0;
}

int next_stop_loc(Path* path, int current) {
    return 0;
}

int reserve_track(int route_server, int train_id, int track_id) {
    TERMmsg msg;
    msg.opcode = RESERVE_NODE;
    msg.param[0] = track_id;
    msg.param[1] = train_id;

    int ret = 0;
    Send(route_server, &msg, sizeof(TERMmsg), &ret, sizeof(int));

    return ret;
}

void release_track(int route_server, int train_id, int track_id) {
    TERMmsg msg;
    msg.opcode = RELEASE_NODE;
    msg.param[0] = track_id;
    msg.param[1] = train_id;

    Send(route_server, &msg, sizeof(TERMmsg), 0, 0);
}

void release_all_track(int route_server, int train_id, int node) {
    TERMmsg msg;
    msg.opcode = RELEASE_ALL;
    msg.param[0] = train_id;
    msg.param[1] = node;

    Send(route_server, &msg, sizeof(TERMmsg), 0, 0);
}

void get_track_reservation(int route_server, int *bitmap) {
    TERMmsg msg;
    msg.opcode = GET_RESERVE_DATA;
    
    Send(route_server, &msg, sizeof(TERMmsg), bitmap, TRACK_BITMAP_MAX * sizeof(int));
}

void path_to_route(Path* path, Route* route) {
    int i;
    route->route_len = 0;
    for (i = 0; i < TRACK_MAX; i++) {
        route->nodes[i].node = 0;    
        route->nodes[i].action = 0;    
        route->nodes[i].arc_dist = 0;
        route->nodes[i].bitmap = 0;    
    }

    if (path->path_len == 0)
        return;

    for (i = path->path_len - 1; i >= 1; i --) {
        route->nodes[route->route_len].node = path->nodes[i];
        switch(path->edges[i - 1]) { // how to reach the next node
            case 0: //DIRAHEAD/STRAIGHT
                if (path->nodes[i]->type == NODE_BRANCH) {
                    route->nodes[route->route_len].action = 1;
                }
                route->nodes[route->route_len].arc_dist = path->nodes[i]->edge[DIR_AHEAD].dist;
                break;
            case 1:
                route->nodes[route->route_len].action = 2;
                route->nodes[route->route_len].arc_dist = path->nodes[i]->edge[DIR_CURVED].dist;
                break;
            case 2:
                //TODO: handle reverse
                break;
        } 
        route->route_len++;
    }
    route->nodes[route->route_len].node = path->nodes[i];
    route->nodes[route->route_len].action = 4;

    route->route_len++;
}

/*
    int current - furthest node that has being reserved and executed
    int lookahead - lookahead distance, quit after first time it reaches negative
*/
int lookahead_node(Route* route, int current, int lookahead, int location_server, int route_server, int train_id) {
    while (current < route->route_len) {
        // if need to reserve
        if (!(route->nodes[current].bitmap & ROUTE_NODE_RESERVED)) {
            int tries = 0;
            while (reserve_track(route_server, train_id, route->nodes[current].node->id) == 0) {
                if (tries == 0) {// first try failed, stop train
                    train_set_speed(location_server, train_id, 0);               
                }
                else if (tries > 3) { // try kept failing, restart route
                    pprintf(COM2, "\033[%d;%dH\033[KReservation fail %s.\n\r", 35 + line++ % 10, 1, route->nodes[current].node->name);
                    return -1;
                }
                tries ++;
                // wait a second to try again
                Delay(100);
            }
            if (tries > 1) {
                train_set_speed(location_server, train_id, 8);
            }
            route->nodes[current].bitmap |= ROUTE_NODE_RESERVED;
        }
        
        // if need to do action
        if ((route->nodes[current].bitmap & ROUTE_NODE_ACTION_COMPLETED) == 0 && route->nodes[current].action > 0) {
            if (route->nodes[current].action < 3) { // turn switch
                pprintf(COM2, "\033[%d;%dH\033[KMake switch %d to %d.\n\r", 35 + line++ % 10, 1, route->nodes[current].node->num, route->nodes[current].action - 1 );
                track_set_switch(location_server, route->nodes[current].node->num, route->nodes[current].action - 1, 1);
            } else { // stop train
                pprintf(COM2, "\033[%d;%dH\033[KStop train at %d.", 35 + line++ % 10, 1, route->nodes[current].node->id);
                stop_train_at(location_server, train_id, route->nodes[current].node->id, route->nodes[current].action == 3? 35 : 15);
            }

            route->nodes[current].bitmap |= ROUTE_NODE_ACTION_COMPLETED;
        }

        // Compute successor
        if (current == route->route_len - 1)
            return 1;

        if (lookahead == 0)
            break;
        else if (lookahead > 0) {
            lookahead -= route->nodes[current].arc_dist;
            if (lookahead < 0)
                lookahead = 0;
        }
        current++;
    }
    return 0;    
}

void lookbehind_node(Route* route, int current, int lookbehind, int route_server, int train_id) {
    while (current > 0) {
        lookbehind -= route->nodes[current - 1].arc_dist;
        current --;

        if (lookbehind < 0) {
            break;
        }
    }

    if (lookbehind >= 0)
        return;

    while (current >= 0) {
        if (route->nodes[current].bitmap & ROUTE_NODE_RESERVED) {
            release_track(route_server, train_id, route->nodes[current].node->id);
            route->nodes[current].bitmap &= ~ROUTE_NODE_RESERVED;
            current --;   
        } else {
            break;
        }
    }
}
