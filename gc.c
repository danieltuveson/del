#include "dsalloc.h"
#include "linkedlist.h"
#include "heap_ptr.h"
#include "gc.h"

/*
struct PointerRemap {
    size_t old_ptr;
    size_t new_ptr;
};

typedef struct LinkedList PointerRemaps;

struct GarbageCollector {
    DSAllocator allocator;
    size_t current_loc;
    struct LinkedListNode *next_remap;
    PointerRemaps *map;
};
*/

void gc_init(struct GarbageCollector *gc)
{
    gc->allocator = dsalloc_new();
    gc->current_loc = 0;
    gc->next_remap = NULL;
    gc->map = linkedlist_new(gc->allocator);
}

void gc_free(struct GarbageCollector *gc)
{
    dsalloc_delete(gc->allocator);
}

void gc_remap(struct GarbageCollector *gc, size_t old_ptr)
{
    size_t old_count = get_count(old_ptr);
    HeapPointer new_ptr = gc->current_loc;
    set_count_no_check(&new_ptr, old_count);

    struct PointerRemap *remap = dsalloc(gc->allocator, sizeof(*remap));
    remap->old_ptr = old_ptr;
    remap->new_ptr = new_ptr;
    linkedlist_append(gc->map, remap);
    gc->current_loc += old_count;
}

void print_remap(struct GarbageCollector *gc)
{
    printf("{\n");
    linkedlist_foreach(lnode, gc->map->head) {
        if (lnode != gc->map->head) printf(",\n");
        struct PointerRemap *map = lnode->value;
        printf("%lu -> %lu", get_location(map->old_ptr), get_location(map->new_ptr));
    }
    printf("\n}\n");
}

void gc_remap_ptr(struct GarbageCollector *gc, HeapPointer *ptr)
{
    assert(ptr != NULL);
    if (gc->next_remap == NULL) gc->next_remap = gc->map->head;
    struct PointerRemap *pr = gc->next_remap->value;
    assert(*ptr == pr->old_ptr);
    *ptr = pr->new_ptr;
}

