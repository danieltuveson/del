#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "functiontable.h"
#include "typecheck.h"

enum Code {
    PUSH,
    PUSH_HEAP,
    AND,
    OR,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
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
    EXIT,
    GET_LOCAL,
    SET_LOCAL,
    DEFINE,
    GET_HEAP,
    SET_HEAP,
    CALL,
    SWAP,
    PUSH_SCOPE,
    POP_SCOPE,
    PRINT
};

typedef union {
    int64_t integer;
    size_t offset;
    double floating;
    char character;
    enum Code opcode;
} DelValue;

struct CompilerContext {
    DelValue *instructions;
    size_t offset;
    struct FunctionCallTable *funcall_table;
    struct ClassTable *class_table;
};

size_t compile(struct CompilerContext *cc, TopLevelDecls *tlds);

#endif
