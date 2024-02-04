#ifndef VM_H
#define VM_H

#include "common.h"

#define STACK_SIZE 256
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

// struct HeapVal {
//     uint64_t size;
//     uint64_t *object;
// };

struct Locals {
    Symbol names[HEAP_SIZE];
    uint64_t values[HEAP_SIZE];
    int count;
    int stack_depth;
};

/* The stack stores almost all data used by the VM */
struct Stack {
    uint64_t offset;
    uint64_t values[STACK_SIZE];
};

/* A value on the heap consists of a "count" prefixing the number of values for that object
 * For example, if you wanted to push a struct of { 4.5, 6.8 } onto a new heap, it
 * the heap would become [ 2, 4.5, 6.8 ]. If you pushed {45, 68, 90}, it would become
 * [ 2, 4.5, 6.8, 3, 45, 68, 90 ]
 */
struct Heap {
    int objcount;
    int offset;
    uint64_t values[HEAP_SIZE];
};

long vm_execute(struct Locals *locals, uint64_t *instructions);

#endif
