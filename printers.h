#ifndef PRINTERS_H
#define PRINTERS_H

#include "parser.h"
#include "compiler.h"
#include "vm.h"

void print_expr(struct Expr *expr, int depth);
void print_exprs(struct Exprs *exprs);
void print_instructions(void **instructions, int length);
void print_stack(struct Stack *stack);
void print_heap(struct Heap *heap);

#endif

