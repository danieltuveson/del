#ifndef VM_H
#define VM_H

#include "common.h"
#include "del.h"

// Struct of arrays storing stack frames
// - names: stores "hashes" of variable names.
// - values: stores corresponding values (names[i] corresponds to values[i]).
// - index: is just the current index of the top of the names / values stacks.
// - frame_offsets: stores a list of the starting indices of each stack frame. 
// - frame_offsets_index: stores the index of the top of frame_start
struct StackFrames {
    size_t index;
    DelValue *values;
    size_t frame_offsets_index;
    size_t *frame_offsets;
};

/* The stack stores almost all data used by the VM */
struct Stack {
    size_t offset;
    DelValue *values;// [STACK_MAX];
};

/* A value on the heap is just a slice of bytes */
struct Heap {
    size_t objcount;
    struct Vector *vector;
    // size_t offset; 
    // DelValue values[HEAP_MAX];
};

// int64_t integer;
// size_t offset;
// double floating;
// char chars[8];
// enum Code opcode;

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
    size_t scope_offset;
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
