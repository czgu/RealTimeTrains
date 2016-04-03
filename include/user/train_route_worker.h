#ifndef _TRAIN_ROUTE_WORKER_H_
#define _TRAIN_ROUTE_WORKER_H_

#include <dijkstra.h>

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
    int status_code;
    int info;
} RouteStatus;

void train_route_worker();
int lookahead_node(Route* route, int current, int lookahead, int location_server, int route_server, int train_id);
void lookbehind_node(Route* route, int current, int lookbehind, int route_server, int train_id);

#endif
