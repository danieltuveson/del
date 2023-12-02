#ifndef BYTECODE_H
#define BYTECODE_H

#include "parser.h"

#define STACK_SIZE  10

enum Code {
    PUSH,
    ADD,
    SUB,
    MUL,
    DIV,
    EQ,
    NEQ,
    LT,
    GT,
    JE,
    JMP,
    RET,
    DEF,
    LOAD
};

int compile(void **instructions, struct Expr *expr, int offset);

#endif
