#ifndef GC_H
#define GC_H

#include "allocator.h"
#include "linkedlist.h"

struct PointerRemap {
    size_t old_ptr;
    size_t new_ptr;
};

typedef struct LinkedList PointerRemaps;

struct GarbageCollector {
    Allocator allocator;
    size_t current_loc;
    struct LinkedListNode *next_remap;
    PointerRemaps *map;
};

void gc_init(struct GarbageCollector *gc);
void gc_free(struct GarbageCollector *gc);
void gc_remap(struct GarbageCollector *gc, size_t old_ptr);
void print_remap(struct GarbageCollector *gc);
void gc_remap_ptr(struct GarbageCollector *gc, HeapPointer *ptr);

#endif

