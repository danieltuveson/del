#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"

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
    JNE,
    JMP,
    POP,
    RET,
    DEF,
    LOAD
};

int compile(void **instructions, Statements *stmts, int offset);

#endif
