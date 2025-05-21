#pragma once

#define FOREACH(node, list) for (node = list->head; node != NULL; node = node->next)

typedef struct ListNode {
    int value;
    struct ListNode* next;
} ListNode;

typedef struct {
    ListNode* head;
    int size;
} List;

List* List_create();

void List_add(List* list, int value);

void List_append(List* list, int value);

void List_insert(List* list, ListNode* node, int value);

void List_remove(List* list, int value);

int List_pop(List* list);

void List_clear(List* list);

void List_delete(List* list);

bool List_find(List* list, int value);

ListNode* List_get(List* list, int index);
