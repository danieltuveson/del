#ifndef LINKEDLIST_H
#define LINKEDLIST_H

struct LinkedListNode;

struct LinkedListNode {
    void *value;
    struct LinkedListNode *prev;
    struct LinkedListNode *next;
};

struct LinkedList {
    unsigned long length;
    struct LinkedListNode *head;
    struct LinkedListNode *tail;
};

#define linkedlist_foreach(lnode, start_node) \
    for (struct LinkedListNode *lnode = start_node; lnode != NULL; lnode = lnode->next)

struct LinkedList *linkedlist_new(void);
void linkedlist_append(struct LinkedList *ll, void *value);
void linkedlist_print(struct LinkedList *ll, void (*printer)(void *));
void linkedlist_reverse(struct LinkedList **ll_ptr);

#endif
