#include "common.h"

// "Generic" hash table

struct HashTableNode {
    uint32_t hash;
    void *value;
};

// { "key": some_object, "anotherkey": some_other_object };

void hashtable_init(void)
{
}

void hashtable_set(uint64_t key, void *value)
{
}

void *hashtable_get(uint64_t key)
{
}

// void hashtable_delete(