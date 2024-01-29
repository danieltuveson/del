#ifndef VM_H
#define VM_H

#include "common.h"

#define STACK_SIZE 256
#define HEAP_SIZE  1024

// enum HeapValType {
//     L_INT,
//     L_FLOAT,
//     L_STRING,
//     L_BOOL,
//     L_OBJ
// };
// 
// struct HeapVal {
//     enum HeapValType type;
//     union {
//         long integer;
//         double floating;
//         char *string;
//         long boolean;
//         void *obj;
//     };
// };

struct Locals {
    Symbol names[HEAP_SIZE];
    uint64_t values[HEAP_SIZE];
    int count;
    int stack_depth;
};

struct Stack {
    int offset;
    uint64_t values[STACK_SIZE];
};

/* A value on the heap consists of a "count" prefixing the number of values to return */
// struct Heap {
//     uint64_t heap[HEAP_SIZE];
// };

// struct Heap {
//     Symbol name;
//     uint64_t value;
//     struct Heap *next;
// };

long vm_execute(struct Locals *locals, uint64_t *instructions);

#endif
