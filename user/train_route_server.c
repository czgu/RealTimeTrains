#include <train_route_server.h>
#include <syscall.h>

#include <train_location_server_task.h>
#include <terminal_mvc_server.h>

#include <io.h>

#include <train.h>
#include <string.h>
#include <dijkstra.h>

int execute_path(Path* path, int train_id, int location_server, int route_server, TrainModelPosition* position);

int line = 0;

void train_route_server() {
    RegisterAs("Route Server");

    int sender;
    TERMmsg request_msg;

    // reserve nodes
    char reserved_nodes[TRACK_MAX];
    memset(reserved_nodes, 0, TRACK_MAX*sizeof(char));

    for (;;) {
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode){
                case RESERVE_NODE: {
                    int success = 0;

                    if (reserved_nodes[(int)request_msg.param[0]] == 0) {
                        reserved_nodes[(int)request_msg.param[0]] = request_msg.param[1];

                        track_node* reverse_node = train_track[(int)request_msg.param[0]].reverse;
                        if (reverse_node != (void *)0) {
                            reserved_nodes[reverse_node->id] = request_msg.param[1];
                        }
                    

                        success = 1;
                    } 

                    Reply(sender, &success, sizeof(int));
                    break;
                }
                case RELEASE_NODE: {
                    Reply(sender, 0, 0);
                    if (reserved_nodes[(int)request_msg.param[0]] == request_msg.param[1]) {
                        reserved_nodes[(int)request_msg.param[0]] = 0;
                        track_node* reverse_node = train_track[(int)request_msg.param[0]].reverse;
                        if (reverse_node != (void *)0) {
                            reserved_nodes[reverse_node->id] = 0;
                        }
                    }
                    break;
                }
                case RELEASE_ALL: {
                    Reply(sender, 0, 0);
                    int i;
                    for (i = 0; i < TRACK_MAX; i++) {
                        if (reserved_nodes[i] == request_msg.param[0]) {
                            reserved_nodes[i] = 0;
                        }
                    }
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

    pprintf(COM2, "\033[%d;%dH\033[KRoute start", 45 + line++ % 10, 1);

    char reserved_nodes[TRACK_MAX];
    memset(reserved_nodes, 0, TRACK_MAX*sizeof(char));    

    int train_id = instruction[0];
    int dest_idx = instruction[1];

    TrainModelPosition position;

    int location_server = WhoIs("Location Server");
    int route_server = WhoIs("Route Server");

    train_set_speed(location_server, train_id, 0);
    Delay(300);

    if (where_is(location_server, train_id, &position) <= 0) {
        pprintf(COM2, "\033[%d;%dH\033[KCan't find train %d.", 45 + line++ % 10, 1, train_id);
        return;
    }
    Path path;

    int path_status = 1;
    while (path_status) {
        dijkstra_find(position.arc->src, train_track + dest_idx, &path, reserved_nodes);

        pprintf(COM2, "\033[%d;%dH\033[K Path: ", 33, 1);
        int i;
        for (i = path.path_len - 1; i >= 0; i--) {
            pprintf(COM2, "%s (%d) -> ", path.nodes[i]->name, path.edges[i]);
        }
        PutStr(COM2, " END\n\r");

        int current_path = path.path_len - 1;
        if (current_path > 1) {
            train_set_speed(location_server, train_id, 8);
            stop_train_at(location_server, train_id, dest_idx, 0);
            path_status = execute_path(&path, train_id, location_server, route_server, &position);
        }
        else if (current_path < 0) {
            pprintf(COM2, "\033[%d;%dH\033[KNo path. ", 35, 1);
            break;
        }

    }

    pprintf(COM2, "\033[%d;%dH\033[KRoute end", 45 + line++ % 10, 1);
}

int execute_path(Path* path, int train_id, int location_server, int route_server, TrainModelPosition* position) {
    //int time = Time();
    int current_path = path->path_len - 1;
    
    //int i; 

    for (;;) {
        if (where_is(location_server, train_id, position) <= 0) {
            pprintf(COM2, "\033[%d;%dH\033[K Cant find train %d", 45 + line++ % 10, 1, train_id);
            Delay(25);
            continue;
        }

        for (; current_path >= 0; current_path--) {
            if (path->nodes[current_path] == position->arc->src)
                break;
        }

        if (current_path < 0) {
            pprintf(COM2, "\033[%d;%dH\033[KCan't find node %s.", 45 + line++ % 10, 1, position->arc->src->name );
            return 1; 
        }

        if (current_path >= 3 && path->nodes[current_path - 2]->type == NODE_BRANCH) {
            if ((path->edges[current_path - 3] & (0x1 << 6)) == 0) {
                pprintf(COM2, "\033[%d;%dH\033[KSet switch %d to %d.", 45 + line++ % 10, 1, path->nodes[current_path - 2]->num, path->edges[current_path - 3]);
                track_set_switch(location_server, path->nodes[current_path - 2]->num, path->edges[current_path - 3], 1);
                path->edges[current_path - 3] |= (0x1 << 6);
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

void release_all_track(int route_server, int train_id) {
    TERMmsg msg;
    msg.opcode = RELEASE_ALL;
    msg.param[0] = train_id;

    Send(route_server, &msg, sizeof(TERMmsg), 0, 0);
}

void path_to_route(Path* path, RoutePath* route) {
    int i;
    route->route_len = 0;
    for (i = 0; i < TRACK_MAX; i++) {
        route->nodes[i].node = 0;    
        route->nodes[i].action = 0;    
        route->nodes[i].arc_dist = 0;    
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
}
