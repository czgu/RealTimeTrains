#include <train_route_reservation_server.h>
#include <train_route_scheduler_server.h>
#include <train_route_worker.h>

#include <syscall.h>

#include <train_location_server_task.h>
#include <terminal_mvc_server.h>

#include <io.h>
#include <terminal_gui.h>

#include <train.h>
#include <string.h>

#include <assert.h>

void bitmap_reserve(int* map, int node_id, int bit) {
    if (node_id >= TRACK_MAX)
        return;

    //track_node* reverse_node = train_track[node_id].reverse;
    //int reverse_node_id = reverse_node != (void *)0 ? reverse_node->id : -1;

    if (bit == 0) {
        map[node_id/sizeof(int)] &= ~(0x1 << (node_id % sizeof(int)));
        //if (reverse_node_id >= 0) {
        //    map[reverse_node_id/sizeof(int)] &= ~(0x1 << (reverse_node_id % sizeof(int)));
        //}
    } else {
        map[node_id/sizeof(int)] |= (0x1 << (node_id % sizeof(int)));
        //if (reverse_node_id >= 0) {
        //    map[reverse_node_id/sizeof(int)] |= (0x1 << (reverse_node_id % sizeof(int)));
        //}
    }
}

void train_route_worker() {
    char instruction[2];
    int sender;

    Receive(&sender, instruction, sizeof(char) * 3);
    Reply(sender, 0, 0);

    int time = Time(); int tid = MyTid();

    debugf("[%d,%d]Route start", time, tid);
    //pprintf(COM2, "\033[%d;%dH\033[K[%d,%d]Route start",35 + line++ % 20, 1, time, tid);

    int track_taken[TRACK_BITMAP_MAX];
    memset(track_taken, 0, TRACK_BITMAP_MAX*sizeof(int));    

    int train_id = instruction[0];
    int dest_idx = instruction[1];

    TrainModelPosition position;

    int scheduler_server = MyParentTid();
    int location_server = WhoIs("Location Server");
    int reservation_server = WhoIs("Route Reservation");

    /*
    train_set_speed(location_server, train_id, 0);
    Delay(300);
    */

    if (where_is(location_server, train_id, &position) <= 0) {
        debugf("Can't find train %d. ", train_id);
        return;
    }

    Path path; // path in graph theory model
    Route route; // route for actual execution

    RouteStatus route_status = {0, 0, 0};

    int tries = 0;

    while (!route_status.completed) {
        //pprintf(COM2, "\033[%d;%dH\033[K[%d]Compute path %d.",35 + line++ % 20, 1, time, route_status.code);
        // turn off destination bit
        //track_taken[position.arc->dest->id/sizeof(int)] &= ~(0x1 << (position.arc->dest->id % 32));
        if (route_status.code == 1) {
            //pprintf(COM2, "\033[%d;%dH\033[KReservation prevent %d.\n\r", 35 + line++ % 20, 1, route_status.info);
            bitmap_reserve(track_taken, route_status.info, 1);
        }

        dijkstra_find(
            position.arc->src, 
            train_track + dest_idx, 
            &path, 
            track_taken);

        if (route_status.code == 1) {
            bitmap_reserve(track_taken, route_status.info, 0);
        }

        path_to_route(&path, &route);

        debugf("Path [%d] : ", train_id);
        int i;
        for (i = 0; i < route.route_len; i++) {
            debugf("%s (%d) -> ", route.nodes[i].node->name, route.nodes[i].action );
        }
        PutStr(COM2, " END\n\r");

        // Start the actual route finding
        if (route.route_len <= 1) {
            debugf("[%d] No path. ", time);
        } else {
            execute_route(
                &route,
                train_id,
                location_server,
                reservation_server,
                &position,
                &route_status);

            // Release the root except the current node
            train_set_speed(location_server, train_id, 0);
            Delay(250);
        }
        tries ++;


        if (tries > 4) {
            debugf("[%d]Route find failed", time);
            break;
        }
    }

    where_is(location_server, train_id, &position);
    release_all_track(reservation_server, train_id, position.arc);

    driver_completed(scheduler_server, train_id, 0);
    debugf("Route end %d", route_status.code);

}

void execute_route(
    Route* route, 
    int train_id, 
    int location_server, 
    int reservation_server, 
    TrainModelPosition* position, 
    RouteStatus* route_status
) {
    track_edge* first_arc = route_get_first_arc(route);
    if (first_arc != (void *)0)
        release_all_track(reservation_server, train_id, first_arc);

    //int time = Time();
    //debugf("\033[%d;%dH\033[KExecute route begin", 35 + line++ % 20, 1, train_id);

    int current_route = 0;
    int route_len = route->route_len;
    //int i; 
    int lookbehind = 300;
    
    int stop_pos = 0;

    if (route->nodes[current_route].action == 3) {
        train_reverse(location_server, train_id);
        current_route ++;
        Delay(20);
    }
             
    train_set_speed(location_server, train_id, 8);

    for (;;) {
        where_is(location_server, train_id, position);

        for (current_route = (current_route < 10 ? 0 : (current_route - 10)); current_route < route_len; current_route++) {
            if (route->nodes[current_route].node == position->arc->src)
                break;
        }

        if (current_route >= route_len) {
            //pprintf(COM2, "\033[%d;%dH\033[KCan't find node %s.", 35 + line++ % 20, 1, position->arc->src->name);
            RETURN_ROUTE(2, 0, 0);
        } else if (current_route == route_len - 1) {
            //train_set_speed(location_server, train_id, 0);

            // if the train is not moving
            if (location_query(location_server, 1, train_id) == 0) {
                RETURN_ROUTE(0, 1, 0);
            }
        }

        // release track
        lookbehind_node(
            route,
            current_route,
            lookbehind,
            reservation_server,
            train_id);
        
        // reserve track and do action
        int lookahead = position->stop_dist + 200 
                      + (int)position->dist_travelled;
        lookahead_node(
            route,
            current_route,
            lookahead,
            location_server,
            reservation_server,
            train_id,
            route_status);

        if (route_status->completed) {
            int wait_time = 1000;
            while(where_is(location_server, train_id, position) > 0 && wait_time > 0) {
                if (position->stop_node_dist == -1) {
                    break;
                }
                else {
                    wait_time -= 50;
                    Delay(50);
                }
            }
            if (wait_time > 0) {
                return;
            } else {
                RETURN_ROUTE(255, 0, 0);
            }
        }

        if (route_status->code != 0) {
            return;
        }

        // set stop marker
        if (position->stop_node_dist == -1) {
            track_node* node = route_get_stop_node(route, &stop_pos);
            stop_pos++;

            if (node != (void *)0)
                stop_train_at(location_server, train_id, node->id, 0);
        }

        Delay(5);
    }
    RETURN_ROUTE(255, 0, 0);
}

/*
    int current - furthest node that has being reserved and executed
    int lookahead - lookahead distance, quit after first time it reaches negative
*/
void lookahead_node(
    Route* route, 
    int current, 
    int lookahead, 
    int location_server, 
    int reservation_server, 
    int train_id, 
    RouteStatus* route_status
) {
    int reached_zero = 0;
    while (current < route->route_len) {
        // if need to reserve
        if (!(route->nodes[current].bitmap & ROUTE_NODE_RESERVED)) {
            int tries = 0;
            while (reserve_track(reservation_server, train_id, route->nodes[current].node->id) == 0) {
                if (tries == 0) {// first try failed, stop train
                    train_set_speed(location_server, train_id, 0);               
                }
                else if (tries > 3) { // try kept failing, restart route
                    //pprintf(COM2, "\033[%d;%dH\033[KReservation fail %s.\n\r", 35 + line++ % 20, 1, route->nodes[current].node->name);
                    RETURN_ROUTE(1, 0, route->nodes[current].node->id);
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
        int action = route->nodes[current].action;
        if ((route->nodes[current].bitmap & ROUTE_NODE_ACTION_COMPLETED) == 0 && action > 0) {
            if (action < 3) { // turn switch
                debugf("Make switch %d to %d.", route->nodes[current].node->num, route->nodes[current].action - 1 );
                track_set_switch(
                    location_server,
                    route->nodes[current].node->num,
                    route->nodes[current].action - 1, 1);
            }
            route->nodes[current].bitmap |= ROUTE_NODE_ACTION_COMPLETED;
        }

        // Compute successor
        if (current == route->route_len - 1) {
            RETURN_ROUTE(0, 1, 0);
        }

        lookahead -= route->nodes[current].arc_dist;

        if (lookahead < 0) {
            if (reached_zero) {
                break;
            } else {
                reached_zero = 1;
            }
        }
        current++;
    }
    RETURN_ROUTE(0, 0, 0);
}

void lookbehind_node(Route* route, int current, int lookbehind, int reservation_server, int train_id) {
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
            release_track(reservation_server, train_id, route->nodes[current].node->id);
            route->nodes[current].bitmap &= ~ROUTE_NODE_RESERVED;
            current --;   
        } else {
            break;
        }
    }
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
                route->nodes[route->route_len].action = 3;
                route->nodes[route->route_len].arc_dist = 0;
                break;
        } 
        route->route_len++;
    }
    route->nodes[route->route_len].node = path->nodes[i];
    route->nodes[route->route_len].action = 4;

    route->route_len++;
}

track_edge* route_get_first_arc(Route* route) {
    track_edge* ret = (void *)0;
    
    int i, action;
    for (i = 0; i < route->route_len - 1; i++) {
        action = route->nodes[i].action;
        if (action < 3) {
            ret = route->nodes[i].node->edge + (action > 0 ? action - 1: action);
            break;
        }
    }
    return ret;
}

track_node* route_get_stop_node(Route* route, int* pos) {
    track_node* node = (void *)0;

    int i;
    for (i = *pos; i < route->route_len; i++) {
        if (route->nodes[i].action == 4) {
            node = route->nodes[i].node;
            break;
        }
    }
    *pos = i;

    return node;
}

void set_route_status(RouteStatus* rs, int code, int completed, int info) {
    rs->code = code;
    rs->completed = completed;
    rs->info = info;
}
