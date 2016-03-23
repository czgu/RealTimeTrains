#ifndef _TRAIN_ROUTE_SERVER_H_
#define _TRAIN_ROUTE_SERVER_H_

#include <dijkstra.h>
#include <track_data.h>

typedef enum {
    RESERVE_NODE,
    RELEASE_NODE,
    RELEASE_ALL,
    GET_RESERVE_DATA
} ROUTE_OP;


typedef struct RouteNode {
    track_node* node;
    char action;
    /*
        0 - no action
        1 - switch straight
        2 - switch curved
        3 - stop, reverse
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

void train_route_server();
void train_route_worker();

int reserve_track(int route_server, int train_id, int track_id);
void release_track(int route_server, int train_id, int track_id);
void release_all_track(int route_server, int train_id, int node);
void get_track_reservation(int route_server, int *bitmap);

int next_stop_loc(Path* path, int current);
int lookahead_node(Route* route, int current, int lookahead, int location_server, int route_server, int train_id);
void lookbehind_node(Route* route, int current, int lookbehind, int route_server, int train_id);



#endif
