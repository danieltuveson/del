#ifndef VM_H
#define VM_H

#define STACK_SIZE  1000

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
