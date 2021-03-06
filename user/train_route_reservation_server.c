#include <train_route_reservation_server.h>
#include <syscall.h>
#include <terminal_mvc_server.h>
#include <courier_warehouse_task.h>

#include <io.h>
#include <terminal_gui.h>
#include <assert.h>
#include <string.h>

int reserve_node(int node_id, int train_id, char* reserved_nodes, int force) {
    ASSERTP(0 <= node_id && node_id < TRACK_MAX, "invalid node %d", node_id);
    ASSERT(reserved_nodes != (void *)0);
    if (reserved_nodes[node_id] == 0 || force == 1) {
        reserved_nodes[node_id] = train_id;

        track_node* reverse_node = train_track[node_id].reverse;
        if (reverse_node != (void *)0) {
            reserved_nodes[reverse_node->id] = train_id;
        }

        return 0;
    } else if (reserved_nodes[node_id] == train_id) {
        return 0;
    } 
    debugf("Failed alloc: %d train: %d, owner: %d", 
            node_id, train_id, reserved_nodes[node_id]);
    // return train to blame
    return reserved_nodes[node_id];
}


void train_route_reservation_server() {
    RegisterAs("Route Reservation");

    // reserve nodes
    char reserved_nodes[TRACK_MAX];
    memset(reserved_nodes, 0, TRACK_MAX*sizeof(char));

    // create courier to send print stuff to view server
    int view_server = WhoIs("View Server");
    ASSERTP(view_server > 0, "view server pid %d", view_server);
    int courier = Create(14, courier_task);         // TODO: change priority?
    int courier_ready = 0;
    Send(courier, &view_server, sizeof(int), 0, 0);

    // print buffer 
    TERMmsg print_pool[40];
    RQueue print_buffer;
    rq_init(&print_buffer, print_pool, 40, sizeof(TERMmsg));
    TERMmsg print_msg;

    int sender;
    TERMmsg request_msg;

    for (;;) {
        sender = 0;
        int sz = Receive(&sender, &request_msg, sizeof(TERMmsg));
        ASSERTP(sender != 0, "%d, unexpected sender value", sender);
        if (sz >= sizeof(char)) {
            switch(request_msg.opcode) {
                case COURIER_NOTIF:
                    courier_ready = 1;
                    break;
                case RESERVE_NODE: {
                    int node_id = (int)request_msg.param[0];
                    int train_id = (int)request_msg.param[1];

                    if (node_id < 0 || node_id > TRACK_MAX)
                        break;

                    int err = reserve_node(node_id, train_id, reserved_nodes, 0);
                    Reply(sender, &err, sizeof(int));
                    
                    // send to print server
                    print_msg.opcode = DRAW_TRAIN_TRACK_ALLOC;
                    print_msg.param[0] = train_id;
                    print_msg.param[1] = node_id;
                    print_msg.param[2] = err;
                    rq_push_back(&print_buffer, &print_msg);

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

                        // send to print server
                        print_msg.opcode = DRAW_TRAIN_TRACK_DEALLOC;
                        print_msg.param[0] = train_id;
                        print_msg.param[1] = node_id;
                        rq_push_back(&print_buffer, &print_msg);
                    }

                    //pprintf(COM2, "\033[%d;%dH\033[K Free: %d train: %d", 25 + line ++ % 10, 1,  request_msg.param[0], request_msg.param[1]);
                    break;
                }
                case PRINT_TRAIN_ALLOC: {
                    // Print all the nodes that this train owns
                    Reply(sender, 0, 0);
                    int train_id = (int)request_msg.param[0];

                    //if (TRAIN_ID_MIN <= train_id && train_id <= TRAIN_ID_MAX) {
                        // We don't want to print duplicate nodes (ie. reverse),
                        // so print the node with the lower id
                        int MAX_NUM_CHARS = 11;          // size of TERMmsg param array + 2 (overwrite extra)
                        //int MAX_NUM_CHARS = 7;
                        print_msg.opcode = DRAW_TRAIN_TRACK_ALLOC_ALL;
                        print_msg.extra = 0;
                        
                        int i, k = 2;   // start k at 2 (param[0], param[1] reserved)
                        for (i = 0; i < TRACK_MAX; i++) {
                            if (reserved_nodes[i] == train_id) {
                                track_node* reverse_node = train_track[i].reverse;
                                if (reverse_node == (void *)0 || reverse_node->id > i) {
                                    ASSERTP(reverse_node == (void *)0
                                            || reserved_nodes[reverse_node->id] == train_id,
                                            "(%s, %s) alloc to %d, %d",
                                            train_track[i].name, reverse_node->name,
                                            train_id, reserved_nodes[reverse_node->id]);
                                    print_msg.param[k] = i;
                                    k++;
                                    if (k >= MAX_NUM_CHARS) {
                                        debugf("PRINT_TRAIN_ALLOC maxed num chars");
                                        break;
                                    }
                                }
                            }
                        }
                        print_msg.param[0] = train_id;
                        print_msg.param[1] = k - 2;     // number of nodes
                        rq_push_back(&print_buffer, &print_msg);
                    //}
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
                    //Delay(200);

                    ASSERT(node_edge->src != (void *)0);
                    ASSERT(node_edge->dest != (void *)0);
                    reserve_node(node_edge->src->id, train_id, reserved_nodes, 1);
                    reserve_node(node_edge->dest->id, train_id, reserved_nodes, 1);

                    // send to print server
                    print_msg.opcode = DRAW_TRAIN_TRACK_DEALLOC;
                    print_msg.param[0] = train_id;
                    print_msg.param[1] = -1;                    // -1 means dealloc all
                    rq_push_back(&print_buffer, &print_msg);
                    break;
                }
                case GET_RESERVE_DATA: {
                    Reply(sender, reserved_nodes, TRACK_MAX * sizeof(char));
                    break;
                }
            }   // end-switch

            if (courier_ready > 0 && !rq_empty(&print_buffer)) {
                print_msg = *((TERMmsg *)rq_pop_front(&print_buffer));
                Reply(courier, &print_msg, sizeof(TERMmsg));

                courier_ready = 0;
            }
        }   // end-if
    }   // end-for
}   // end-function

int reserve_track(int reservation_server, int train_id, int track_id) {
    TERMmsg msg;
    msg.opcode = RESERVE_NODE;
    msg.param[0] = track_id;
    msg.param[1] = train_id;

    int ret = 0;
    Send(reservation_server, &msg, sizeof(TERMmsg), &ret, sizeof(int));
    if (ret > 0) {
        // ret is the id of the train who owns this node
        // print all the nodes that train owns
        msg.opcode = PRINT_TRAIN_ALLOC;
        msg.param[0] = ret;
        Send(reservation_server, &msg, sizeof(TERMmsg), 0, 0);
    }

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

