#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "common.h"

struct LinkedListNode;

struct LinkedListNode {
    void *value;
    struct LinkedListNode *prev;
    struct LinkedListNode *next;
};

struct LinkedList {
    Allocator allocator;
    unsigned long length;
    struct LinkedListNode *head;
    struct LinkedListNode *tail;
};

#define linkedlist_foreach(lnode, start_node) \
    for (struct LinkedListNode *lnode = start_node; lnode != NULL; lnode = lnode->next)

#define linkedlist_foreach_reverse(lnode, start_node) \
    for (struct LinkedListNode *lnode = start_node; lnode != NULL; lnode = lnode->prev)

#define linkedlist_vforeach_help(lnode, val, list)\
    for (struct LinkedListNode *lnode = list->head;\
            lnode != NULL && ((val =  lnode->value) || 1);\
            lnode = lnode->next)

#define linkedlist_vforeach(val, list) \
    linkedlist_vforeach_help( ( lnode_ ## __COUNTER__ ), val, list)

#define linkedlist_vforeach_reverse_help(lnode, val, list) \
    for (struct LinkedListNode *lnode = list->tail;\
            lnode != NULL && ((val = lnode->value) || 1);\
            lnode = lnode->prev)

#define linkedlist_vforeach_reverse(val, list) \
    linkedlist_vforeach_reverse_help( ( lnode_ ## __COUNTER__ ), val, list)

struct LinkedList *linkedlist_new(Allocator a);
void linkedlist_append(struct LinkedList *ll, void *value);
void linkedlist_prepend(struct LinkedList *ll, void *value);
void *linkedlist_pop(struct LinkedList *ll);
void linkedlist_print(struct LinkedList *ll, void (*printer)(void *));
void linkedlist_reverse(struct LinkedList **ll_ptr);
bool linkedlist_is_empty(struct LinkedList *ll);

#endif
