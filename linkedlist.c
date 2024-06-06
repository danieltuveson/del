#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"

struct LinkedList *linkedlist_new(void)
{
    struct LinkedList *ll = malloc(sizeof(*ll));
    ll->length = 0;
    ll->head = NULL;
    ll->tail = NULL;
}

void linkedlist_append(struct LinkedList *ll, void *value)
{
    struct LinkedListNode *lnode = malloc(sizeof(*lnode));
    lnode->prev = NULL;
    lnode->next = NULL;
    lnode->value = value;
    ll->length++;
    if (ll->head == NULL) {
        ll->head = lnode;
        ll->tail = lnode;
    } else {
        struct LinkedListNode *oldtail = ll->tail;
        ll->tail = lnode;
        oldtail->next = ll->tail;
        ll->tail->prev = oldtail;
    }
}

void linkedlist_reverse(struct LinkedList **ll_ptr)
{
    if ((*ll_ptr)->head == NULL) {
        return;
    }
    struct LinkedList *old_ll = *ll_ptr;
    for (struct LinkedListNode *lnode = old_ll->tail; lnode != NULL; lnode = lnode->next) {
        struct LinkedListNode *tmp = lnode->next; 
        lnode->next = lnode->prev;
        lnode->prev = tmp;
    }
    struct LinkedListNode *old_head = old_ll->head;
    (*ll_ptr)->head = (*ll_ptr)->tail;
    (*ll_ptr)->tail = old_head;
}

void linkedlist_print(struct LinkedList *ll, void (*printer)(void *))
{
    printf("[ ");
    linkedlist_foreach(lnode, ll->head) {
        printer(lnode->value);
        if (lnode->next != NULL) printf(", ");
    }
    printf(" ]\n");
}
