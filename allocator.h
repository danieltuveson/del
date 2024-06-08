#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include "common.h"

void *allocator_malloc(size_t size);
void allocator_freeall(void);
void print_memory_usage(void);

#endif
