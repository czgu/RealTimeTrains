#ifndef _TRAIN_ROUTE_RESERVATION_SERVER_H_
#define _TRAIN_ROUTE_RESERVATION_SERVER_H_

#include <track_data.h>

typedef enum {
    RESERVE_NODE,
    RELEASE_NODE,
    RELEASE_ALL,
    GET_RESERVE_DATA
} ROUTE_OP;
// Route server
void train_route_reservation_server();

int reserve_track(int route_server, int train_id, int track_id);
void release_track(int route_server, int train_id, int track_id);
void release_all_track(int route_server, int train_id, track_edge* current_arc);
void get_track_reservation(int route_server, char *reserved_nodes);

#endif
