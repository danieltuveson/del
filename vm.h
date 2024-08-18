#ifndef VM_H
#define VM_H

#include "common.h"

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

// Stores info about local variables
// struct StackFrame {
//     Symbol name;
//     uint64_t value;
//     int count;
//     int frame_start;
// };

// Struct of arrays storing stack frames
// - names: stores "hashes" of variable names.
// - values: stores corresponding values (names[i] corresponds to values[i]).
// - index: is just the current index of the top of the names / values stacks.
// - frame_offsets: stores a list of the starting indices of each stack frame. 
// - frame_offsets_index: stores the index of the top of frame_start
struct StackFrames {
    size_t index;
    // Symbol names[HEAP_SIZE];
    uint64_t values[HEAP_SIZE];
    size_t frame_offsets_index;
    size_t frame_offsets[HEAP_SIZE];
};

/* The stack stores almost all data used by the VM */
struct Stack {
    uint64_t offset;
    uint64_t values[STACK_SIZE];
};

#define COUNT_OFFSET    32
#define METADATA_OFFSET 48
#define METADATA_MASK   (UINT64_MAX - ((UINT64_C(1) << METADATA_OFFSET) - 1))
#define LOCATION_MASK   ((UINT64_C(1) << COUNT_OFFSET) - 1)
#define COUNT_MASK      (UINT64_MAX - (LOCATION_MASK + METADATA_MASK))

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
    int objcount;
    int offset;
    uint64_t values[HEAP_SIZE];
};

int vm_execute(uint64_t *instructions);

#endif
