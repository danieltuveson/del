#ifndef FUNCTIONTABLE_H
#define FUNCTIONTABLE_H
#include "common.h"

/* Stores every location in the instructions where each function is called.
 * Since we don't know the exact location in memory where a function will live
 * until after it is compiled, we'll need to walk through this structure after
 * the initial compilation to fill in all of the callsites.
 *
 * Using a binary tree to store this information.
 */
struct FunctionTable {
    struct FunctionTableNode *node;
    struct FunctionTable *left;
    struct FunctionTable *right;
};

struct FunctionTableNode {
    Symbol function;
    uint64_t location;
    struct List *callsites;
};

struct FunctionTable *new_ft(uint64_t function);
struct FunctionTableNode *add_ft_node(struct FunctionTable *ft, Symbol function, uint64_t loc);
void add_callsite(struct FunctionTable *ft, Symbol function, uint64_t callsite);

#endif
