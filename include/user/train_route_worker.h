#ifndef _TRAIN_ROUTE_WORKER_H_
#define _TRAIN_ROUTE_WORKER_H_

#include <dijkstra.h>
#include <train.h>

typedef struct RouteNode {
    track_node* node;
    char action;
    /*
        0 - no action
        1 - switch straight
        2 - switch curved
        3 - reverse
        4 - stop (destination)
    */
    // dist to next node
    int arc_dist;

    #define ROUTE_NODE_RESERVED 0x1
    #define ROUTE_NODE_ACTION_COMPLETED 0x2
    char bitmap;
} RouteNode;

typedef struct Route {
    RouteNode nodes[TRACK_MAX];
    short route_len;
} Route;

void path_to_route(Path* path, Route* route);
track_edge* route_get_first_arc(Route* route);
track_node* route_get_stop_node(Route* route, int* pos);

typedef struct RouteStatus {
    /*
        0 - good
        1 - Aborted due to failed allocation
        2 - Aborted due to lost
        3 - Aborted due to new destination
        255 - Aborted due to unknown error
    */
    int code;
    int completed;
    int info;
} RouteStatus;

#define RETURN_ROUTE(a,b,c) set_route_status(route_status, a,b,c);return

void set_route_status(RouteStatus* rs, int code, int completed, int info);

void train_route_worker();
void execute_route(Route* route, int train_id, int location_server, int reservation_server, TrainModelPosition* position, RouteStatus* route_status);
void lookahead_node(Route* route, int current, int lookahead, int location_server, int reservation_server, int train_id, RouteStatus* route_status);
void lookbehind_node(Route* route, int current, int lookbehind, int route_server, int train_id);

#endif
