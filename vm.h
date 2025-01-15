#ifndef VM_H
#define VM_H

#include "common.h"
#include "del.h"

// Struct of arrays storing stack frames
struct StackFrames {
    size_t index;
    DelValue *values;
    size_t frame_offsets_index;
    size_t *frame_offsets;
};

/* The stack stores almost all data used by the VM */
struct Stack {
    size_t offset;
    DelValue *values;
};

/* A value on the heap is just a slice of bytes */
struct Heap {
    size_t gc_threshold;
    struct Vector *vector;
};

typedef struct {
    Type type;
    DelValue value;
} HeapValue;

struct VirtualMachine {
    enum VirtualMachineStatus status;
    struct StackFrames sfs;
    struct Stack stack;
    struct Heap heap;
    size_t ip;
    // size_t scope_offset;
    uint64_t ret;
    DelValue val1;
    DelValue val2;
    size_t iterations;
    DelValue *instructions;
};

void vm_init(struct VirtualMachine *vm, DelValue *instructions);
void vm_free(struct VirtualMachine *vm);
uint64_t vm_execute(struct VirtualMachine *vm);

#endif

