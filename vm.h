#ifndef VM_H
#define VM_H

#define STACK_SIZE  256
#define LOCALS_SIZE 256
#define GLOBALS_SIZE 256


struct Locals {
    char *names[LOCALS_SIZE];
    void *values[LOCALS_SIZE];
};

struct Globals {
    char *names[LOCALS_SIZE];
    void *values[LOCALS_SIZE];
};

struct Stack {
    int offset;
    void *values[STACK_SIZE];
};

struct Heap {
    char *name;
    long value;
    struct Heap *next;
};

long vm_execute(struct Heap *heap, void **instructions);


#endif
