#include "vector.h"
#include "compiler.h"
#include "common.h"

#define LIST_GROWTH_FACTOR 2
#define LIST_SHRINK_FACTOR LIST_GROWTH_FACTOR * LIST_GROWTH_FACTOR

void vector_print(struct Vector *vector)
{
    printf(
            "max_capacity: %lu\n"
            "min_capacity: %lu\n"
            "capacity: %lu\n"
            "length: %lu\n"
            "values: { ",
            vector->max_capacity,
            vector->min_capacity,
            vector->capacity,
            vector->length
          );
    for (size_t i = 0; i < vector->length; i++) {
        printf("%ld, ", vector->values[i].integer);
    }
    printf("| ");
    for (size_t i = vector->length; i < vector->capacity; i++) {
        printf("%ld, ", vector->values[i].integer);
    }
    printf("}\n");

}

struct Vector *vector_new(size_t init_capacity, size_t max_capacity)
{
    assert(init_capacity <= max_capacity);
    assert(init_capacity > 0);
    struct Vector *vector = malloc(sizeof(*vector));
    vector->max_capacity = max_capacity;
    vector->min_capacity = init_capacity;
    vector->capacity = init_capacity;
    vector->length = 0;
    vector->values = calloc(init_capacity, sizeof(*(vector->values)));
    return vector;
}

void vector_free(struct Vector *vector)
{
    free(vector->values);
    free(vector);
}

static void vector_grow(struct Vector **vector_ptr, size_t new_capacity)
{
    struct Vector *old_vector = *vector_ptr;
    *vector_ptr = vector_new(new_capacity, old_vector->max_capacity);
    (*vector_ptr)->length = old_vector->length;
    (*vector_ptr)->min_capacity = old_vector->min_capacity;
    memcpy((*vector_ptr)->values, old_vector->values, sizeof(DelValue) * old_vector->length);
    vector_free(old_vector);
}

struct Vector *vector_append(struct Vector **vector_ptr, DelValue value)
{
    struct Vector *vector = *vector_ptr;
    if (vector->length == vector->capacity) {
        size_t new_capacity = vector->capacity * LIST_GROWTH_FACTOR;
        if (new_capacity > vector->max_capacity && vector->max_capacity > 0) {
            return NULL;
        }
        vector_grow(vector_ptr, new_capacity);
        vector = *vector_ptr;
    }
    vector->values[vector->length] = value;
    vector->length++;
    return vector;
}

static inline void vector_shrink_internal(struct Vector **vector_ptr)
{
    struct Vector *vector = *vector_ptr;
    if (LIST_SHRINK_FACTOR * vector->length <= vector->capacity) {
        size_t new_capacity = vector->capacity / LIST_GROWTH_FACTOR;
        if (new_capacity >= vector->min_capacity) {
            vector_grow(vector_ptr, new_capacity);
        }
    }
}

DelValue vector_pop(struct Vector **vector_ptr)
{
    assert((*vector_ptr)->length > 0);
    struct Vector *vector = *vector_ptr;
    vector->length--;
    DelValue value = vector->values[vector->length];
    vector_shrink_internal(vector_ptr);
    return value;
}

void vector_shrink(struct Vector **vector_ptr, size_t n)
{
    struct Vector *vector = *vector_ptr;
    assert(n <= vector->length);
    vector->length -= n;
    vector_shrink_internal(vector_ptr);
}

