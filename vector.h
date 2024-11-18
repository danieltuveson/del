#ifndef VECTOR_H
#define VECTOR_H

#include "common.h"
#include "compiler.h"

#define VECTOR_DEFAULT_INIT 8

struct Vector {
    size_t max_capacity; // if max_capacity given is 0, allow vector to grow unbounded
    size_t min_capacity; // defaults to initial capacity
    size_t capacity; 
    size_t length;
    DelValue *values;
};

struct Vector *vector_new(size_t init_capacity, size_t max_capacity);
void vector_free(struct Vector *vector);
struct Vector *vector_append(struct Vector **vector_ptr, DelValue value);
DelValue vector_pop(struct Vector **vector_ptr);
void vector_shrink(struct Vector **vector_ptr, size_t n);
void vector_grow(struct Vector **vector_ptr, size_t n);
void vector_print(struct Vector *vector);

static inline DelValue vector_index(struct Vector *vector, size_t i)
{
    assert(i < vector->length);
    return vector->values[i];
}

#endif
