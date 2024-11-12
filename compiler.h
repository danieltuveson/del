#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "functiontable.h"
#include "typecheck.h"

#include "bytecode.h"

typedef union {
    int64_t integer;
    size_t offset;
    double floating;
    char chars[8];
    enum Code opcode;
    Type type;
} DelValue;

struct CompilerContext {
    DelValue *instructions;
    size_t offset;
    struct FunctionCallTable *funcall_table;
    struct ClassTable *class_table;
};

size_t compile(struct Globals *globals, TopLevelDecls *tlds);

#endif
