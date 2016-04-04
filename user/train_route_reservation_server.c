#include <train_route_reservation_server.h>
#include <syscall.h>
#include <terminal_mvc_server.h>

#include <io.h>
#include <assert.h>
#include <string.h>

static int line = 0;

int reserve_node(int node_id, int train_id, char* reserved_nodes, int force) {
    if (reserved_nodes[node_id] == 0 || force == 1) {
        reserved_nodes[node_id] = train_id;

        track_node* reverse_node = train_track[node_id].reverse;
        if (reverse_node != (void *)0) {
            reserved_nodes[reverse_node->id] = train_id;
        }

        return 1;
    } else if (reserved_nodes[node_id] == train_id) {
        return 1;
    } 
    pprintf(COM2, "\033[%d;%dH\033[K Failed alloc: %d train: %d, owner: %d", 25 + line ++ % 10, 1,  node_id, train_id, reserved_nodes[node_id]);
    return 0;
}


void train_route_reservation_server() {
    RegisterAs("Route Reservation");

    // reserve nodes
    char reserved_nodes[TRACK_MAX];
    memset(reserved_nodes, 0, TRACK_MAX*sizeof(char));

    int sender;
    TERMmsg request_msg;

    for (;;) {
        sender = 0;
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        ASSERTP(sender != 0, "%d, unexpected sender value", sender);
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case RESERVE_NODE: {
                    int node_id = (int)request_msg.param[0];
                    int train_id = (int)request_msg.param[1];

                    if (node_id < 0 || node_id > TRACK_MAX)
                        break;

                    int success = reserve_node(node_id, train_id, reserved_nodes, 0);
                    Reply(sender, &success, sizeof(int));
                    break;
                }
                case RELEASE_NODE: {
                    Reply(sender, 0, 0);

                    int node_id = (int)request_msg.param[0];
                    int train_id = (int)request_msg.param[1];

                    if (node_id < 0 || node_id > TRACK_MAX)
                        break;

                    if (reserved_nodes[node_id] == train_id) {
                        reserve_node(node_id, 0, reserved_nodes, 1);
                    }

                    //pprintf(COM2, "\033[%d;%dH\033[K Free: %d train: %d", 25 + line ++ % 10, 1,  request_msg.param[0], request_msg.param[1]);
                    break;
                }
                case RELEASE_ALL: {
                    Reply(sender, 0, 0);
                    //pprintf(COM2, "\033[%d;%dH\033[K release all train: %d, node %d, sender %d %d", 35 + line ++ % 20, 1,  request_msg.param[0], request_msg.param[1], sender, s );
                    int i;

                    int train_id = (int)request_msg.param[0];
                    track_edge* node_edge = (track_edge *)request_msg.extra;

                    for (i = 0; i < TRACK_MAX; i++) {
                        if (reserved_nodes[i] == train_id) {
                            reserved_nodes[i] = 0;
                        }
                    }

                    if (node_edge == (void *)0)
                        break;
                    // pprintf(COM2, "\033[%d;%dH\033[K release all train: %s to %s", 35 + line ++ % 20, 1, node_edge->src->name, node_edge->dest->name);
                    Delay(200);

                    reserve_node(node_edge->src->id, train_id, reserved_nodes, 1);
                    reserve_node(node_edge->dest->id, train_id, reserved_nodes, 1);
                    break;
                }
                case GET_RESERVE_DATA: {
                    Reply(sender, reserved_nodes, TRACK_MAX * sizeof(char));
                    break;
                }
            }
        }
    }
}

int reserve_track(int reservation_server, int train_id, int track_id) {
    TERMmsg msg;
    msg.opcode = RESERVE_NODE;
    msg.param[0] = track_id;
    msg.param[1] = train_id;

    int ret = 0;
    Send(reservation_server, &msg, sizeof(TERMmsg), &ret, sizeof(int));

    return ret;
}

void release_track(int reservation_server, int train_id, int track_id) {
    TERMmsg msg;
    msg.opcode = RELEASE_NODE;
    msg.param[0] = track_id;
    msg.param[1] = train_id;

    Send(reservation_server, &msg, sizeof(TERMmsg), 0, 0);
}

void release_all_track(int reservation_server, int train_id, track_edge* current_arc) {
    TERMmsg msg;
    msg.opcode = RELEASE_ALL;
    msg.param[0] = train_id;
    msg.extra = (unsigned int)current_arc;

    Send(reservation_server, &msg, sizeof(TERMmsg), 0, 0);
}

void get_track_reservation(int reservation_server, char *reserved_nodes) {
    TERMmsg msg;
    msg.opcode = GET_RESERVE_DATA;
    
    Send(reservation_server, &msg, sizeof(TERMmsg), reserved_nodes, TRACK_MAX * sizeof(char));
}
