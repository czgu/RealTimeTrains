#include <dijkstra.h>

#include <io.h>

inline static void min_heap_swap(MinHeap* heap, int idx1, int idx2) {
    NodePair* temp = heap->nodes[idx1];
    heap->nodes[idx1] = heap->nodes[idx2];
    heap->nodes[idx2] = temp;

    heap->nodes[idx1]->index = idx1;
    heap->nodes[idx2]->index = idx2;
}

inline static void min_heap_bubble_down(MinHeap* heap, int index) {
    int left, right, smallest;
    while(1) {
        left = 2 * index + 1;
        right = 2 * index + 2;
        smallest = index;

        if (left < heap->size && heap->nodes[left]->dist < heap->nodes[smallest]->dist) {
            smallest = left;    
        }
        if (right < heap->size && heap->nodes[right]->dist < heap->nodes[smallest]->dist) {
            smallest = right;
        }

        //bwprintf(COM2, "\tbubble down %s, %d from %d to %d\n\r", heap->nodes[index]->node->name, heap->nodes[index]->dist, index, smallest );

        if (smallest != index) {
            min_heap_swap(heap, smallest, index);
            index = smallest;
        } else {
            break;
        }
    }
}

inline static void min_heap_bubble_up(MinHeap* heap, int index) {
    int parent = (index - 1)/2;

    while (index > 0 && heap->nodes[parent]->dist > heap->nodes[index]->dist) {
        //bwprintf(COM2, "\tbubble up %s, %d from %d to %d\n\r", heap->nodes[index]->node->name, heap->nodes[index]->dist, index, parent );
        min_heap_swap(heap, parent, index);
        index = parent;
        parent = (index - 1)/2;
    }
}

void min_heap_decrease_key(MinHeap* heap, int index, int val) {
    //bwprintf(COM2, "decrease key %s from %d to %d\n\r", heap->nodes[index]->node->name, heap->nodes[index]->dist, val);

    if (index < heap->size) {
        heap->nodes[index]->dist = val;
        min_heap_bubble_up(heap, index);
    }
}

NodePair* min_heap_extract_min(MinHeap* heap) {
    NodePair* ret = (void*)0;

    if (heap->size > 0) {
        ret = heap->nodes[0];
        ret->index = -1;

        heap->size --;
        heap->nodes[0] = heap->nodes[heap->size];

        min_heap_bubble_down(heap, 0);        
    }

    return ret;
}


void dijkstra_init_heap(MinHeap* heap, NodePair* nodes) {
    int i;

    for (i = 0; i < TRACK_MAX; i++) {
        nodes[i].node = train_track + i;
        nodes[i].prev = (void*)0;
        nodes[i].prev_edge = -1;
        nodes[i].dist = (0x1 << 30);
        heap->nodes[i] = nodes + i;
        nodes[i].index = i;
    }
    
    heap->size = TRACK_MAX;
}

void dijkstra_find(track_node* src, track_node* dest, Path* path, char* reserved_nodes) {
    NodePair nodes[TRACK_MAX];
    MinHeap heap;
    int alt_dist, id, i;    

    dijkstra_init_heap(&heap, nodes);
    min_heap_decrease_key(&heap, nodes[src->id].index, 0);

    while (heap.size > 0) {
        NodePair* np = min_heap_extract_min(&heap);

        //bwprintf(COM2, "top node: %s dist: %d remaining: %d\n\r", np->node->name, np->dist,heap.size);
        // if not reachable
        if (np->node == dest || np->dist == (0x1 << 30))
            break;
    
        // Decrease successors
        int num_edge = 0;
        int node_type = np->node->type;
        if (node_type == NODE_MERGE || node_type == NODE_SENSOR || node_type == NODE_ENTER) {
            num_edge = 1;
        } else if(node_type == NODE_BRANCH) {
            num_edge = 2;
        }

        // Consider reverse
        alt_dist = np->dist; //TODO: add some reverse distance?
        id = np->node->reverse->id;
        if (alt_dist < nodes[id].dist) {
            nodes[id].prev = np->node;
            nodes[id].prev_edge = 2;
            min_heap_decrease_key(&heap, nodes[id].index, alt_dist);
        }

        // Consider straight edges
        for (i = 0; i < num_edge; i++) {
            id = np->node->edge[i].dest->id;
            alt_dist = np->dist + np->node->edge[i].dist;

            if (alt_dist < nodes[id].dist && (reserved_nodes[np->node->id] & (0x1 << i)) == 0 ) {
                nodes[id].prev = np->node;
                nodes[id].prev_edge = i;
                min_heap_decrease_key(&heap, nodes[id].index, alt_dist);
            }
        }
    }

    path->path_len = 0;
    if (nodes[dest->id].dist != (0x1 << 30)) {
        while (dest != (void*)0) {
            path->nodes[path->path_len] = nodes[dest->id].node;
            path->edges[path->path_len] = nodes[dest->id].prev_edge;
            path->path_len ++;

            dest = nodes[dest->id].prev;
        }
    }
}
