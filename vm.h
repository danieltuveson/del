#ifndef VM_H
#define VM_H

#include "common.h"

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

#define COUNT_OFFSET    UINT64_C(32)
#define METADATA_OFFSET UINT64_C(48)
#define METADATA_MASK   (UINT64_MAX - ((UINT64_C(1) << METADATA_OFFSET) - 1))
#define LOCATION_MASK   ((UINT64_C(1) << COUNT_OFFSET) - 1)
#define COUNT_MASK      (UINT64_MAX - (LOCATION_MASK + METADATA_MASK))
#define GC_MARK_MASK    (UINT64_C(1) << 63)

/* A value on the heap is just a slice of bytes
 * A pointer to the heap consists of 3 parts:
 * - The first byte represents metadata about the heap value. TBD what goes here - I'm going to
 *   include the "mark" portion of mark and sweep gc as one of these bits. I also think I should
 *   include metadata for strings, like how many bytes are in the last uint64.
 * - The next 3 bytes stores the size of the data. For most objects this will be small, but
 *   arrays could possibly use up the full range.
 * - The last 32 bits store the location in the heap: that means our heap can store up to about 
 *   4 gigabytes of data before hitting this limit.
 */
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

enum VirtualMachineStatus {
    VM_STATUS_ERROR = 0, 
    VM_STATUS_COMPLETED = 1, 
    VM_STATUS_PAUSE = 2
};

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
