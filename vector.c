#include "vector.h"
#include "compiler.h"
#include "common.h"

#define LIST_GROWTH_FACTOR 2

void vector_print(struct Vector *vector)
{
    printf(
            "max_capacity: %lu\n"
            "capacity: %lu\n"
            "length: %lu\n"
            "values: { ",
            vector->max_capacity,
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

static struct Vector *vector_grow(struct Vector **vector_ptr)
{
    struct Vector *old_vector = *vector_ptr;
    size_t new_capacity = old_vector->capacity * LIST_GROWTH_FACTOR;
    if (new_capacity > old_vector->max_capacity && old_vector->max_capacity > 0) {
        return NULL;
    }
    *vector_ptr = vector_new(new_capacity, old_vector->max_capacity);
    (*vector_ptr)->length = old_vector->length;
    memcpy((*vector_ptr)->values, old_vector->values, sizeof(DelValue) * old_vector->length);
    vector_free(old_vector);
    return *vector_ptr;
}

struct Vector *vector_append(struct Vector **vector_ptr, DelValue value)
{
    struct Vector *vector = *vector_ptr;
    if (vector->length == vector->capacity) {
        vector = vector_grow(vector_ptr);
        if (vector == NULL) {
            return NULL;
        }
    }
    vector->values[vector->length] = value;
    vector->length++;
    return vector;
}

