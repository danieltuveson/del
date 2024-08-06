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
void print_fundef(struct FunDef *fundef, int indent, int ismethod);
// void print_class(struct Class *cls, int indent);

/* Compiler printers */
void print_ft_node(struct FunctionCallTableNode *fn);
void print_ft(struct FunctionCallTable *ft);

/* VM printers */
void print_instructions(struct CompilerContext *cc);
void print_stack(struct Stack *stack);
void print_heap(struct Heap *heap);
void print_frames(struct StackFrames *sfs);

#endif

