#ifndef VM_H
#define VM_H

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
