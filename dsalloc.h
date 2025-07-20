#ifndef DSALLOC_H
#define DSALLOC_H
#include <stddef.h>
#include <inttypes.h>

typedef uintptr_t DSAllocator;

// Creates a new allocator
DSAllocator dsalloc_new(void);

// Allocates memory of given size in `a`
// All memory allocated by this function is initialized to zeros
void *dsalloc(DSAllocator a, size_t size);

// Frees all memory allocated in `a`
// `a` may *not* be used after delete is called
void dsalloc_delete(DSAllocator a);

// Returns the total memory the user has requested so far (in bytes)
unsigned long dsalloc_memory_usage(DSAllocator a);

// Returns the memory overhead of the allocator itself (in bytes)
// `dsalloc_overhead(a) + dsalloc_memory_usage(a)` gives the total memory used
// by the allocator
unsigned long dsalloc_overhead(DSAllocator a);

#endif

