#ifndef _TRAIN_ROUTE_SERVER_H_
#define _TRAIN_ROUTE_SERVER_H_

#include <dijkstra.h>
#include <track_data.h>

typedef enum {
    RESERVE_NODE,
    RELEASE_NODE,
    RELEASE_ALL
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
} RouteNode;

typedef struct RoutePath {
    RouteNode nodes[TRACK_MAX];
    short route_len;
} RoutePath;

void path_to_route(Path* path, RoutePath* route);

void train_route_server();
void train_route_worker();

int reserve_track(int route_server, int train_id, int track_id);
void release_track(int route_server, int train_id, int track_id);
void release_all_track(int route_server, int train_id);

int next_stop_loc(Path* path, int current);




#endif
