#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>

typedef uintptr_t Allocator;

Allocator allocator_new(void);
void *allocator_malloc(Allocator a, size_t size);
void allocator_freeall(Allocator a);
void print_memory_usage(Allocator a);

#endif
