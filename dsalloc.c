#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "dsalloc.h"

struct Stack;
struct Stack {
    void *memory;
    struct Stack *next;
};

struct Alloc {
    struct Stack *stack;
    unsigned long memory_usage;
    unsigned long overhead;
};

DSAllocator dsalloc_new(void)
{
    struct Alloc *a = malloc(sizeof(*a));
    a->stack = NULL;
    a->memory_usage = 0;
    a->overhead = sizeof(*a);
    return (DSAllocator)a;
}

void *dsalloc(DSAllocator a, size_t size)
{
    struct Alloc *allocator = (struct Alloc *)a;
    allocator->memory_usage += size;
    allocator->overhead += sizeof(struct Stack);

    void *memory = malloc(size);
    memset(memory, 0, size);

    struct Stack *stack = malloc(sizeof(*stack));
    stack->memory = memory;
    stack->next = allocator->stack;
    allocator->stack = stack;

    return memory;
}

void dsalloc_delete(DSAllocator a)
{
    struct Alloc *allocator = (struct Alloc *)a;
    struct Stack *next = NULL;
    for (struct Stack *current = allocator->stack; current != NULL; current = next) {
        next = current->next;
        free(current->memory);
        free(current);
    }
    free(allocator);
}

unsigned long dsalloc_memory_usage(DSAllocator a)
{
    struct Alloc *allocator = (struct Alloc *)a;
    return allocator->memory_usage;
}

unsigned long dsalloc_overhead(DSAllocator a)
{
    struct Alloc *allocator = (struct Alloc *)a;
    return allocator->overhead;
}

