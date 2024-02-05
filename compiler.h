#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "functiontable.h"

struct CompilerContext {
    uint64_t *instructions;
    int offset;
    struct FunctionTable *ft;
};

enum Code {
    PUSH,
    PUSH_HEAP,
    AND,
    OR,
    ADD,
    SUB,
    MUL,
    DIV,
    EQ,
    NEQ,
    LT,
    LTE,
    GT,
    GTE,
    UNARY_PLUS,
    UNARY_MINUS,
    FLOAT_ADD,
    FLOAT_SUB,
    FLOAT_MUL,
    FLOAT_DIV,
    FLOAT_EQ,
    FLOAT_NEQ,
    FLOAT_LT,
    FLOAT_LTE,
    FLOAT_GT,
    FLOAT_GTE,
    FLOAT_UNARY_PLUS,
    FLOAT_UNARY_MINUS,
    JE,
    JNE,
    JMP,
    POP,
    RET,
    DEF,
    SET,
    LOAD,
    CALL,
    GET_HEAP
};

int compile(struct CompilerContext *cc, TopLevelDecls *tlds);

#endif
