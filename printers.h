#ifndef PRINTERS_H
#define PRINTERS_H

#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "functiontable.h"
#include "typecheck.h"

/* AST printers */
void print_expr(struct Globals *globals, struct Expr *expr);
void print_value(struct Globals *globals, struct Value *val);
void print_statement(struct Globals *globals, struct Statement *stmt);
void print_statements(struct Globals *globals, Statements *stmts);
void print_tlds(struct Globals *globals, TopLevelDecls *tlds);
void print_fundef(struct Globals *globals, struct FunDef *fundef, int indent, int ismethod);
// void print_class(struct Globals *globals, struct Class *cls, int indent);

/* Compiler printers */
void print_ft_node(struct Globals *globals, struct FunctionCallTableNode *fn);
void print_ft(struct Globals *globals, struct FunctionCallTable *ft);

/* VM printers */
void print_instructions(struct Vector *instructions);
void print_stack(struct Stack *stack);
void print_frames(struct StackFrames *sfs);
void print_heap(struct Heap *heap);

#endif

