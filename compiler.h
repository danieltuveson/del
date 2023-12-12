#ifndef COMPILER_H
#define COMPILER_H

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
    POP,
    RET,
    DEF,
    LOAD
};

int compile(void **instructions, struct Exprs *exprs, int offset);

#endif
