#include "common.h"
#include "allocator.h"

// A very simple stack-based allocator
// Probably should replace this with an region-based allocator, but for now just doing this

struct Stack;
struct Stack {
    void *memory;
    struct Stack *next;
};

struct Stack *global_allocator = NULL;
size_t global_total_mem_usage = 0;
size_t global_allocator_usage = 0;

void *allocator_malloc(size_t size)
{
    global_total_mem_usage += size;
    // printf("allocating %lu bytes\n", size);
    global_allocator_usage += sizeof(struct Stack);
    void *memory = malloc(size);
    memset(memory, 0, size);
    struct Stack *stack = malloc(sizeof(*stack));
    stack->memory = memory;
    stack->next = global_allocator;
    global_allocator = stack;
    return memory;
}

void allocator_freeall(void)
{
    struct Stack *next = NULL;
    for (struct Stack *current = global_allocator; current != NULL; current = next) {
        next = current->next;
        free(current->memory);
        free(current);
    }
    global_allocator = NULL;
    global_total_mem_usage = 0;
}

void print_memory_usage(void)
{
    printf("%lu bytes of heap-allocated memory\n", global_total_mem_usage);
    printf("%lu bytes of allocator overhead\n", global_allocator_usage);
}
