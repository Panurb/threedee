#pragma once


typedef struct ArrayList {
    void* data;
    int size;
    int capacity;
    int element_size;
} ArrayList;


ArrayList* ArrayList_create(int element_size);

void* ArrayList_get(ArrayList* list, int index);

void ArrayList_add(ArrayList* list, void* element);

void ArrayList_remove(ArrayList* list, int index);

void ArrayList_clear(ArrayList* list);

void ArrayList_destroy(ArrayList* list);

void ArrayList_for_each(ArrayList* list, void (*print_func)(void*));

int ArrayList_find(ArrayList* list, void* value);
