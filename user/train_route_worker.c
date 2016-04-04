#include <train_route_reservation_server.h>
#include <train_route_worker.h>

#include <syscall.h>

#include <train_location_server_task.h>
#include <terminal_mvc_server.h>

#include <io.h>

#include <train.h>
#include <string.h>

#include <assert.h>

int execute_route(Route* route, int train_id, int location_server, int reservation_server, TrainModelPosition* position);

static int line = 0;

void train_route_worker() {
    char instruction[2];
    int sender;

    Receive(&sender, instruction, sizeof(char) * 3);
    Reply(sender, 0, 0);

    int time = Time(); int tid = MyTid();

    pprintf(COM2, "\033[%d;%dH\033[K[%d,%d]Route start",35 + line++ % 20, 1, time, tid);

    int track_taken[TRACK_BITMAP_MAX];
    memset(track_taken, 0, TRACK_BITMAP_MAX*sizeof(int));    

    int train_id = instruction[0];
    int dest_idx = instruction[1];

    TrainModelPosition position;

    int scheduler_server = MyParentTid();
    int location_server = WhoIs("Location Server");
    int reservation_server = WhoIs("Route Reservation");

    train_set_speed(location_server, train_id, 0);
    Delay(300);
    if (where_is(location_server, train_id, &position) <= 0) {
        pprintf(COM2, "\033[%d;%dH\033[KCan't find train %d. ", 35 + line++ % 20, 1, train_id);
        return;
    }

    Path path; // path in graph theory model
    Route route; // route for actual execution

    // 0: good, -1 : bad
    int path_status = -1;

    int tries = 0;

    while (path_status < 0) {
        //pprintf(COM2, "\033[%d;%dH\033[K[%d]Compute path. ",35 + line++ % 20, 1, time);
        // turn off destination bit
        //track_taken[position.arc->dest->id/sizeof(int)] &= ~(0x1 << (position.arc->dest->id % 32));


        dijkstra_find(position.arc->src, train_track + dest_idx, &path, track_taken);
        path_to_route(&path, &route);
        /*
        pprintf(COM2, "\033[%d;%dH\033[K Path [%d] : ", 35 + line++ % 20, 1, train_id);
        int i;
        for (i = 0; i < route.route_len; i++) {
            pprintf(COM2, "%s (%d) -> ", route.nodes[i].node->name, route.nodes[i].action );
        }
        PutStr(COM2, " END\n\r");
        */
        // Start the actual route finding
        if (route.route_len <= 1) {
            pprintf(COM2, "\033[%d;%dH\033[K[%d] No path. ", 35 + line ++ % 10, 1, time);
        } else {
            path_status = execute_route(&route, train_id, location_server, reservation_server, &position);

            // Release the root except the current node
            train_set_speed(location_server, train_id, 0);
            Delay(250);
        }
        tries ++;


        if (tries > 4) {
            pprintf(COM2, "\033[%d;%dH\033[K[%d]Route find failed", 35 + line++ % 20, 1, time);
            break;
        }
    }

    where_is(location_server, train_id, &position);
    release_all_track(reservation_server, train_id, position.arc);

    driver_completed(scheduler_server, train_id, 0);
    pprintf(COM2, "\033[%d;%dH\033[K[%d]Route end", 35 + line++ % 20, 1, time);

}

int execute_route(Route* route, int train_id, int location_server, int reservation_server, TrainModelPosition* position) {
    track_edge* first_arc = route_get_first_arc(route);
    if (first_arc != (void *)0)
        release_all_track(reservation_server, train_id, first_arc);

    //int time = Time();
    //pprintf(COM2, "\033[%d;%dH\033[KExecute route begin", 35 + line++ % 20, 1, train_id);

    int current_route = 0;
    int route_len = route->route_len;
    //int i; 
    int lookahead = 600;
    int lookbehind = 300;
    int status = 0;
    
    int stop_pos = 0;

    if (route->nodes[current_route].action == 3) {
        train_reverse(location_server, train_id);
        current_route ++;
        Delay(20);
    }
             
    train_set_speed(location_server, train_id, 8);

    for (;;) {
        where_is(location_server, train_id, position);

        for (current_route = (current_route < 5 ? 0 : (current_route - 5)); current_route < route_len; current_route++) {
            if (route->nodes[current_route].node == position->arc->src)
                break;
        }

        if (current_route >= route_len) {
            // pprintf(COM2, "\033[%d;%dH\033[KCan't find node %s.", 35 + line++ % 20, 1, position->arc->src->name);
            return -1; 
        } else if (current_route == route_len - 1) {
            //train_set_speed(location_server, train_id, 0);

            // if the train is not moving
            if (location_query(location_server, 1, train_id) == 0) {
                break;
            }
        }

        // release track
        lookbehind_node(route, current_route, lookbehind, reservation_server, train_id);
        
        // reserve track and do action
        status = lookahead_node(route, current_route, lookahead, location_server, reservation_server, train_id);

        if (status != 0) {
            if (status == -1) {
                return -1;
            }

            if(status == 1) {
                while(where_is(location_server, train_id, position) > 0) {
                    if (position->stop_dist == -1)
                        break;
                    else
                        Delay(50);
                }
                //pprintf(COM2, "\033[%d;%dH\033[KExecute_route done.\n\r", 35 + line++ % 20, 1);
                return 0;
            }
        }

        // set stop marker
        if (position->stop_dist == -1) {
            track_node* node = route_get_stop_node(route, &stop_pos);
            stop_pos++;

            if (node != (void *)0)
                stop_train_at(location_server, train_id, node->id, 20);
        }

        Delay(5);
    }
    return 0;
}

/*
    int current - furthest node that has being reserved and executed
    int lookahead - lookahead distance, quit after first time it reaches negative
*/
int lookahead_node(Route* route, int current, int lookahead, int location_server, int reservation_server, int train_id) {
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
        int action = route->nodes[current].action;
        if ((route->nodes[current].bitmap & ROUTE_NODE_ACTION_COMPLETED) == 0 && action > 0) {
            if (action < 3) { // turn switch
                //pprintf(COM2, "\033[%d;%dH\033[KMake switch %d to %d.\n\r", 35 + line++ % 20, 1, route->nodes[current].node->num, route->nodes[current].action - 1 );
                track_set_switch(location_server, route->nodes[current].node->num, route->nodes[current].action - 1, 1);
            }
            route->nodes[current].bitmap |= ROUTE_NODE_ACTION_COMPLETED;
        }

        // Compute successor
        if (current == route->route_len - 1)
            return 1;

        lookahead -= route->nodes[current].arc_dist;
        if (lookahead < 0 && reached_zero)
            break;
        else
            reached_zero = 1;

        current++;
    }
    return 0;    
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

