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
int typecheck(struct Ast *ast, struct Class *clst, struct FunDef *ft);

#endif
