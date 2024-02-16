#include "common.h"
#include "ast.h"
#include "printers.h"

/* Functions to create top level definitions */
struct TopLevelDecl *new_class(Symbol symbol, Definitions *definitions, Methods *methods)
{
    struct TopLevelDecl *tld = malloc(sizeof(struct TopLevelDecl));
    tld->type = TLD_TYPE_CLASS;
    tld->cls = malloc(sizeof(struct Class));
    tld->cls->name = symbol;
    tld->cls->definitions = definitions == NULL ? NULL : reset_list_head(definitions);
    tld->cls->methods = methods == NULL ? NULL : reset_list_head(methods);
    return tld;
}

struct FunDef *new_fundef(Symbol symbol, Definitions *args, Statements *stmts)
{
    struct FunDef *fundef = malloc(sizeof(struct FunDef));
    fundef->name = symbol;
    fundef->args = args;
    fundef->stmts = reset_list_head(stmts);
    return fundef;
}

struct TopLevelDecl *new_tld_fundef(Symbol symbol, Definitions *args, Statements *stmts)
{
    struct TopLevelDecl *tld = malloc(sizeof(struct TopLevelDecl));
    tld->type = TLD_TYPE_FUNDEF;
    tld->fundef = new_fundef(symbol, args, stmts);
    return tld;
}

/* Functions to create Statements */
struct Statement *new_set(Symbol symbol, struct Value *val)
{
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_SET;
    stmt->set = malloc(sizeof(struct Set));
    stmt->set->symbol = symbol;
    stmt->set->val = val;
    return stmt;
}

struct Statement *new_if(struct Value *condition, Statements *if_stmts, Statements *else_stmts) {
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_IF;
    stmt->if_stmt = malloc(sizeof(struct IfStatement));
    stmt->if_stmt->condition = condition;
    stmt->if_stmt->if_stmts = reset_list_head(if_stmts);
    if (else_stmts) {
        stmt->if_stmt->else_stmts = reset_list_head(else_stmts);
    } else {
        stmt->if_stmt->else_stmts = NULL;
    }
    return stmt;
}

struct Statement *new_while(struct Value *condition, Statements *stmts)
{
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_WHILE;
    stmt->while_stmt = malloc(sizeof(struct While));
    stmt->while_stmt->condition = condition;
    stmt->while_stmt->stmts = reset_list_head(stmts);
    return stmt;
}

struct Statement *new_let(Let *let)
{
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_LET;
    stmt->let = reset_list_head(let);
    return stmt;
}
 
struct Definition *new_define(Symbol name, enum Type type)
{
    struct Definition *def = malloc(sizeof(struct Definition));
    def->name = name;
    def->type = type;
    return def;
}

static struct FunCall *new_funcall(Symbol funname, Values *args)
{
    struct FunCall *funcall = malloc(sizeof(struct FunCall));
    funcall->funname = funname;
    funcall->args = args;
    return funcall;
}

struct Statement *new_sfuncall(Symbol funname, Values *args)
{
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_FUNCALL;
    stmt->funcall = new_funcall(funname, args);
    return stmt;
}

struct Statement *new_return(struct Value *val)
{
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_RETURN;
    stmt->ret = val;
    return stmt;
}

/* Functions for creating Values */
struct Value *new_string(char *string)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_STRING;
    val->string = string;
    return val;
}

struct Value *new_symbol(uint64_t symbol)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_SYMBOL;
    val->symbol = symbol;
    return val;
}

struct Value *new_integer(long integer)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_INT;
    val->integer = integer;
    return val;
}

struct Value *new_floating(double floating)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_FLOAT;
    val->floating = floating;
    return val;
}

struct Value *new_boolean(int boolean)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_BOOL;
    val->boolean = boolean;
    return val;
}

struct Value *new_vfuncall(Symbol funname, Values *args)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_FUNCALL;
    val->funcall = new_funcall(funname, args);
    return val;
}

struct Value *new_expr(struct Expr *expr)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_EXPR;
    val->expr = expr;
    return val;
}

/* Macros used to generate functions for creating Expressions.
 * Unary and binary operators have the same structure, so this 
 * avoids us having to copy-paste the same snippets many times */

#define define_unary_op(name,type) \
struct Expr *name(struct Value *val1) {\
    struct Expr *expr = malloc(sizeof(struct Expr));\
    expr->op = type;\
    expr->val1 = val1;\
    expr->val2 = NULL;\
    return expr;\
}

define_unary_op(unary_plus, OP_UNARY_PLUS)
define_unary_op(unary_minus, OP_UNARY_MINUS)

#undef define_unary_op

#define define_binary_op(name,type) \
struct Expr *name(struct Value *val1, struct Value *val2) {\
    struct Expr *expr = malloc(sizeof(struct Expr));\
    expr->op = type;\
    expr->val1 = val1;\
    expr->val2 = val2;\
    return expr;\
}

define_binary_op(bin_or, OP_OR)
define_binary_op(bin_and, OP_AND)
define_binary_op(bin_eqeq, OP_EQEQ)
define_binary_op(bin_not_eq, OP_NOT_EQ)
define_binary_op(bin_greater_eq, OP_GREATER_EQ)
define_binary_op(bin_greater, OP_GREATER)
define_binary_op(bin_less_eq, OP_LESS_EQ)
define_binary_op(bin_less, OP_LESS)
define_binary_op(bin_plus, OP_PLUS)
define_binary_op(bin_minus, OP_MINUS)
define_binary_op(bin_star, OP_STAR)
define_binary_op(bin_slash, OP_SLASH)

#undef define_binary_op

