#include "allocator.h"

// A very simple stack-based allocator
// Probably should replace this with an region-based allocator, but for now just doing this

struct Stack;
struct Stack {
    void *memory;
    struct Stack *next;
};

struct Alloc {
    struct Stack *global_allocator;
    size_t global_total_mem_usage;
    size_t global_allocator_usage;
};


Allocator allocator_new(void)
{
    struct Alloc *a = malloc(sizeof(*a));
    a->global_allocator = NULL;
    a->global_total_mem_usage = 0;
    a->global_allocator_usage = 0;
    return (Allocator)a;
}

void *allocator_malloc(Allocator a, size_t size)
{
    struct Alloc *allocator = (struct Alloc *)a;
    allocator->global_total_mem_usage += size;
    // printf("allocating %lu bytes\n", size);
    allocator->global_allocator_usage += sizeof(struct Stack);
    void *memory = malloc(size);
    memset(memory, 0, size);
    struct Stack *stack = malloc(sizeof(*stack));
    stack->memory = memory;
    stack->next = allocator->global_allocator;
    allocator->global_allocator = stack;
    return memory;
}

void allocator_freeall(Allocator a)
{
    struct Alloc *allocator = (struct Alloc *)a;
    struct Stack *next = NULL;
    for (struct Stack *current = allocator->global_allocator; current != NULL; current = next) {
        next = current->next;
        free(current->memory);
        free(current);
    }
    free(allocator);
}

void print_memory_usage(Allocator a)
{
    struct Alloc *allocator = (struct Alloc *)a;
    printf("%lu bytes of heap-allocated memory\n", allocator->global_total_mem_usage);
    printf("%lu bytes of allocator overhead\n", allocator->global_allocator_usage);
}

