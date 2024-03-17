#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// typedef uint32_t List; // handle to list

#define TYPE double
#define INITIAL_CAPACITY 4

struct List {
    uint64_t length;
    uint64_t capacity;
    TYPE items[];
};

#define base_list_size(type_size) (2 * sizeof(uint64_t) + INITIAL_CAPACITY * (type_size))

// #define define_list_new(allocator, type, typename)

static struct List *list_new(void *(*allocator)(size_t)) {
    struct List *list = allocator(base_list_size(sizeof(TYPE)));
    list->length = 0;

    return list;
}

int main()
{
    printf("hello list\n");
}
