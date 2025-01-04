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

struct Scope {
    bool isfunction;
    size_t varcount;
    Definitions *definitions;
    struct Scope *parent;
};

struct Class *lookup_class(struct ClassTable *ct, Symbol symbol);
bool typecheck(struct Globals *globals);

#endif
