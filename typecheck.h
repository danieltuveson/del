#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "common.h"
#include "ast.h"

struct ClassTable {
    size_t size;
    struct Class *table;
};

struct FunctionTable {
    size_t size;
    struct FunDef *table;
};

struct Class *lookup_class(struct ClassTable *ct, Symbol symbol);
void print_lhs(Symbol symbol, LValues *lvalues, int n);
int typecheck(struct Ast *ast, struct ClassTable *class_table,
        struct FunctionTable *function_table);

#endif
