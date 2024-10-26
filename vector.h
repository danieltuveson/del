#ifndef VECTOR_H
#define VECTOR_H

#include "common.h"
#include "compiler.h"

struct Vector {
    size_t max_capacity; // if max_capacity given is 0, allow vector to grow unbounded
    size_t capacity; 
    size_t length;
    DelValue *values;
};

struct Vector *vector_new(size_t init_capacity, size_t max_capacity);
void vector_free(struct Vector *vector);
struct Vector *vector_append(struct Vector **vector_ptr, DelValue value);
void vector_print(struct Vector *vector);

#endif
