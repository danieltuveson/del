#ifndef PRINTERS_H
#define PRINTERS_H

#include "parser.h"
#include "compiler.h"
#include "vm.h"

/* VM printers */
void print_instructions(void **instructions, int length);
void print_stack(struct Stack *stack);
void print_heap(struct Heap *heap);

/* AST printers */
void print_expr(struct Expr *expr);
void print_value(struct Value *val);
void print_statement(struct Statement *stmt);
void print_statements(Statements *stmts);

#endif

