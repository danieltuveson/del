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

/* Typechecking printers */
void print_scope(struct Globals *globals, struct Scope *scope);

/* Compiler printers */
void print_ft_node(struct Globals *globals, struct FunctionCallTableNode *fn);
void print_ft(struct Globals *globals, struct FunctionCallTable *ft);

/* VM printers */
void print_instructions(struct CompilerContext *cc);
void print_stack(struct Stack *stack, bool is_obj);
void print_frames(struct StackFrames *sfs, bool is_obj);
void print_heap(struct Heap *heap);

/* Misc. */
void print_binary_helper(uint64_t num, size_t length);
#define print_binary(num) print_binary_helper((uint64_t)num, sizeof(num))

#endif

