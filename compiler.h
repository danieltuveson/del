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
    uint16_t types[4];
    enum Code opcode;
    Type type;
} DelValue;

typedef struct LinkedList BreakLocations;

struct CompilerContext {
    struct Vector *instructions;
    BreakLocations *breaks;
    BreakLocations *continues;
    struct FunctionCallTable *funcall_table;
    struct ClassTable *class_table;
};

size_t compile(struct Globals *globals, TopLevelDecls *tlds);

#endif
