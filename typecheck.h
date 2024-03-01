#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "common.h"
#include "ast.h"

struct ClassType {
    Symbol name;
    uint64_t count;
    Type *types;
};

struct FunctionType {
    Symbol name;
    uint64_t count;
    Type *types;
};

struct Scope {
    Definitions *definitions;
    struct Scope *parent;
};

int typecheck(void);

#endif
