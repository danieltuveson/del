#ifndef PRINTERS_H
#define PRINTERS_H

#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "functiontable.h"
#include "typecheck.h"

/* AST printers */
void print_expr(struct Expr *expr);
void print_value(struct Value *val);
void print_statement(struct Statement *stmt);
void print_statements(Statements *stmts);
void print_tlds(TopLevelDecls *tlds);

/* Typechecking printers */
void print_class_table(struct ClassType *table, uint64_t length);
void print_function_table(struct FunctionType *table, uint64_t length);

/* Compiler printers */
void print_ft_node(struct FunctionTableNode *fn);
void print_ft(struct FunctionTable *ft);

/* VM printers */
void print_instructions(struct CompilerContext *cc);
void print_stack(struct Stack *stack);
void print_heap(struct Heap *heap);
void print_locals(struct Locals *locals);

#endif

