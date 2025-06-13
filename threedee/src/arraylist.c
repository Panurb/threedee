#include <stddef.h>

#include "arraylist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"


ArrayList* ArrayList_create(int element_size) {
    ArrayList* list = malloc(sizeof(ArrayList));
    list->capacity = 128;
    list->data = malloc(element_size * list->capacity);
    list->size = 0;
    list->element_size = element_size;
    return list;
}


void* ArrayList_get(ArrayList* list, int index) {
    if (index < 0) {
        index += list->size;
    }
    if (index >= list->size) {
        LOG_ERROR("Index out of bounds: %d (size: %d)", index, list->size);
        return NULL; // Index out of bounds
    }
    return (char*)list->data + index * list->element_size;
}


void ArrayList_add(ArrayList* list, void* element) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->data = realloc(list->data, list->capacity * list->element_size);
    }
    memcpy((char*)list->data + list->size * list->element_size, element, list->element_size);
    list->size++;
}


void ArrayList_remove(ArrayList* list, int index) {
    if (index < 0 || index >= list->size) {
        return; // Index out of bounds
    }
    memmove((char*)list->data + index * list->element_size,
            (char*)list->data + (index + 1) * list->element_size,
            (list->size - index - 1) * list->element_size);
    list->size--;
}


void ArrayList_clear(ArrayList* list) {
    list->size = 0; // Reset size, but keep capacity
}


void ArrayList_destroy(ArrayList* list) {
    free(list->data);
    free(list);
}


void ArrayList_for_each(ArrayList* list, void (*print_func)(void*)) {
    for (int i = 0; i < list->size; i++) {
        print_func((char*)list->data + i * list->element_size);
    }
}


int ArrayList_find(ArrayList* list, void* value) {
    for (int i = 0; i < list->size; i++) {
        if (memcmp((char*)list->data + i * list->element_size, value, list->element_size) == 0) {
            return i;
        }
    }
    return -1;
}
