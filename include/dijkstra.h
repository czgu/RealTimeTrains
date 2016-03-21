#ifndef _DIJKSTRA_H_
#define _DIJKSTRA_H_

#include <track_data.h>

typedef struct NodePair {
    track_node* node;
    track_node* prev;
    char prev_edge;

    int dist;
    short index;
} NodePair;

typedef struct MinHeap {
    NodePair* nodes[TRACK_MAX];
    int size;
} MinHeap;

typedef struct Path {
    int path_len;
    // reversed from dest to src
    track_node* nodes[TRACK_MAX];

    // edges[i]: node(i + 1) -> node(i)
    // 0 - DIR_AHEAD/DIR_STRAIGHT , 1 - DIR_CURVE, 2 REVERSE
    char edges[TRACK_MAX - 1];

} Path;


/* Private
inline static void min_heap_swap(MinHeap* heap, int idx1, int idx2)
inline static void min_heap_bubble_up(MinHeap* heap, int index);
inline static void min_heap_bubble_down(MinHeap* heap, int index);
*/

void min_heap_decrease_key(MinHeap* heap, int index, int val);
NodePair* min_heap_extract_min(MinHeap* heap);

void dijkstra_init_heap(MinHeap* heap, NodePair* nodes);

/*
    Reservation Bitmap
    value
    0 - free
    1 - reservered
    position
    0 - forward/straight edge reservered
    1 - curve edge reservered
*/
void dijkstra_find(track_node* src, track_node* dest, Path* path, char* reserved_nodes);

#endif
