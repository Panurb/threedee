#include <stdlib.h>
#include <stdbool.h>

#include "heap.h"
#include "component.h"


int parent(int i) {
    return i / 2;
}


int left(int i) {
    return 2 * i;
}


int right(int i) {
    return 2 * i + 1;
}


void swap(Heap* heap, int i, int j) {
    int t = heap->array[i];
    heap->array[i] = heap->array[j];
    heap->array[j] = t;
}


bool compare(int i, int j) {
    // WaypointComponent* wi = WaypointComponent_get(i);
    // WaypointComponent* wj = WaypointComponent_get(j);
    // return wi->f_score < wj->f_score;
}


void heapify(Heap* heap, int i) {
    int l = left(i);
    int r = right(i);
    int smallest = i;

    if (l <= heap->size && compare(heap->array[l], heap->array[i])) {
        smallest = l;
    }

    if (r <= heap->size && compare(heap->array[r], heap->array[smallest])) {
        smallest = r;
    }

    if (smallest != i) {
        swap(heap, i, smallest);
        heapify(heap, smallest);
    }
}


Heap* Heap_create() {
    Heap* heap = malloc(sizeof(Heap));
    heap->size = 0;
    return heap;
}


void Heap_destroy(Heap* heap) {
    free(heap);
}


void Heap_insert(Heap* heap, int key) {
    heap->size++;

    int i = heap->size;
    while(i > 1 && compare(key, heap->array[parent(i)])) {
        heap->array[i] = heap->array[parent(i)];
        i = parent(i);
    }

    heap->array[i] = key;
}


int Heap_extract(Heap* heap) {
    int minimum = heap->array[1];

    heap->array[1] = heap->array[heap->size];
    heap->size--;

    heapify(heap, 1);

    return minimum;
}


int Heap_find(Heap* heap, int key) {
    for (int i = 1; i <= heap->size; i++) {
        if (heap->array[i] == key) {
            return i;
        }
    }
    return -1;
}
