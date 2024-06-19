#ifndef FUNCTIONTABLE_H
#define FUNCTIONTABLE_H
#include "common.h"
#include "linkedlist.h"

/* Stores every location in the instructions where each function is called.
 * Since we don't know the exact location in memory where a function will live
 * until after it is compiled, we'll need to walk through this structure after
 * the initial compilation to fill in all of the callsites.
 *
 * Using a binary tree to store this information.
 */
struct FunctionCallTable {
    struct FunctionCallTableNode *node;
    struct FunctionCallTable *left;
    struct FunctionCallTable *right;
};

struct FunctionCallTableNode {
    Symbol function;
    uint64_t location;
    struct LinkedList *callsites;
};

struct FunctionCallTable *new_ft(uint64_t function);
struct FunctionCallTableNode *add_ft_node(struct FunctionCallTable *ft, Symbol function,
        uint64_t loc);
void add_callsite(struct FunctionCallTable *ft, Symbol function, uint64_t callsite);

#endif
