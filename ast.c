#include "common.h"
#include "allocator.h"
// #include "printers.h"
#include "linkedlist.h"
#include "ast.h"

/* Functions to create top level definitions */
static struct TopLevelDecl *new_tld(enum TLDType tld_type)
{
    struct TopLevelDecl *tld = allocator_malloc(sizeof(struct TopLevelDecl));
    tld->type = tld_type;
    return tld;
}

struct TopLevelDecl *new_class(Symbol symbol, Definitions *definitions, Methods *methods)
{
    globals.class_count++;
    struct TopLevelDecl *tld = new_tld(TLD_TYPE_CLASS);
    tld->cls = allocator_malloc(sizeof(struct Class));
    tld->cls->name = symbol;
    tld->cls->definitions = definitions == NULL ? NULL : definitions;
    tld->cls->methods = methods == NULL ? NULL : methods;
    return tld;
}

struct Definition *lookup_property(struct Class *cls, Symbol name)
{
    linkedlist_foreach(lnode, cls->definitions->head) {
        struct Definition *def = lnode->value;
        if (def->name == name) {
            return def;
        }
    }
    return NULL;
}

// Code assumes that property name does exist
uint64_t lookup_property_index(struct Class *cls, Symbol name)
{
    uint64_t cnt = 0;
    linkedlist_foreach(lnode, cls->definitions->head) {
        struct Definition *def = lnode->value;
        if (def->name == name) {
            return cnt;
        }
        cnt++;
    }
    assert("Error: lookup_property_index could not find element" && 0);
    return 0;
}

static struct Accessor *new_accessor(Symbol symbol, LValues *lvalues)
{
    struct Accessor *accessor = allocator_malloc(sizeof(struct Accessor));
    accessor->definition = new_define(symbol, TYPE_UNDEFINED);
    accessor->lvalues = lvalues;
    return accessor;
}

struct FunDef *new_fundef(Symbol symbol, Type rettype, Definitions *args, Statements *stmts)
{
    struct FunDef *fundef = allocator_malloc(sizeof(struct FunDef));
    fundef->name = symbol;
    fundef->rettype = rettype;
    fundef->args = args;
    fundef->stmts = stmts;
    return fundef;
}

struct TopLevelDecl *new_tld_fundef(Symbol symbol, Type rettype, Definitions *args,
        Statements *stmts)
{
    globals.function_count++;
    struct TopLevelDecl *tld = new_tld(TLD_TYPE_FUNDEF);
    tld->fundef = new_fundef(symbol, rettype, args, stmts);
    return tld;
}

/* Functions to create Statements */
static struct Statement *new_stmt(enum StatementType st)
{
    struct Statement *stmt = allocator_malloc(sizeof(struct Statement));
    stmt->type = st;
    return stmt;
}

struct Statement *new_set(Symbol symbol, struct Value *val, LValues *lvalues, bool is_define)
{
    struct Statement *stmt = new_stmt(STMT_SET);
    stmt->set = allocator_malloc(sizeof(struct Set));
    stmt->set->to_set = new_accessor(symbol, lvalues);
    stmt->set->is_define = is_define;
    stmt->set->val = val;
    return stmt;
}

struct Statement *new_if(struct Value *condition, Statements *if_stmts)
{
    struct Statement *stmt = new_stmt(STMT_IF);
    stmt->if_stmt = allocator_malloc(sizeof(struct IfStatement));
    stmt->if_stmt->condition = condition;
    stmt->if_stmt->if_stmts = if_stmts;
    stmt->if_stmt->else_stmts = NULL;
    return stmt;
}

// This is a little hacky, maybe re-write the IfStatement structure to accomodate else-ifs
struct Statement *add_elseif(struct IfStatement *if_stmt, struct Statement *elseif_stmt)
{
    if_stmt->else_stmts = linkedlist_new();
    linkedlist_append(if_stmt->else_stmts, elseif_stmt);
    return if_stmt->else_stmts->head->value;
}

void add_else(struct IfStatement *if_stmt, Statements *else_stmts)
{
    if_stmt->else_stmts = else_stmts;
}

struct Statement *new_while(struct Value *condition, Statements *stmts)
{
    struct Statement *stmt = new_stmt(STMT_WHILE);
    stmt->while_stmt = allocator_malloc(sizeof(struct While));
    stmt->while_stmt->condition = condition;
    stmt->while_stmt->stmts = stmts;
    return stmt;
}

struct Statement *new_for(struct Statement *init, struct Value *condition,
        struct Statement *increment, Statements *stmts)
{
    struct Statement *stmt = new_stmt(STMT_FOR);
    stmt->for_stmt = allocator_malloc(sizeof(struct For));
    stmt->for_stmt->init = init;
    stmt->for_stmt->condition = condition;
    stmt->for_stmt->increment = increment;
    stmt->for_stmt->stmts = stmts;
    return stmt;
}

struct Statement *new_let(Definitions *let)
{
    struct Statement *stmt = new_stmt(STMT_LET);
    stmt->let = let;
    return stmt;
}
 
struct Definition *new_define(Symbol name, Type type)
{
    struct Definition *def = allocator_malloc(sizeof(struct Definition));
    def->name = name;
    def->type = type;
    def->scope_offset = 0;
    return def;
}

static struct FunCall *new_funcall(Symbol funname, Values *args)
{
    struct FunCall *funcall = allocator_malloc(sizeof(struct FunCall));
    funcall->funname = funname;
    funcall->args = args;
    return funcall;
}

struct Statement *new_sfuncall(Symbol funname, Values *args, bool is_builtin)
{
    enum StatementType t = is_builtin ? STMT_BUILTIN_FUNCALL : STMT_FUNCALL;
    struct Statement *stmt = new_stmt(t);
    stmt->funcall = new_funcall(funname, args);
    return stmt;
}

struct Statement *new_return(struct Value *val)
{
    struct Statement *stmt = new_stmt(STMT_RETURN);
    stmt->ret = val;
    return stmt;
}

/* Functions for creating Values */
static struct Value *new_value(enum ValueType vtype, Type type)
{
    struct Value *val = allocator_malloc(sizeof(struct Value));
    val->vtype = vtype;
    val->type = type;
    return val;
}

struct Value *new_string(char *string)
{
    struct Value *val = new_value(VTYPE_STRING, TYPE_STRING);
    val->string = string;
    return val;
}

struct Value *new_integer(long integer)
{
    struct Value *val = new_value(VTYPE_INT, TYPE_INT);
    val->integer = integer;
    return val;
}

struct Value *new_floating(double floating)
{
    struct Value *val = new_value(VTYPE_FLOAT, TYPE_FLOAT);
    val->floating = floating;
    return val;
}

struct Value *new_boolean(int boolean)
{
    struct Value *val = new_value(VTYPE_BOOL, TYPE_BOOL);
    val->boolean = boolean;
    return val;
}

struct Value *new_null(void)
{
    struct Value *val = new_value(VTYPE_NULL, TYPE_NULL);
    val->integer = 0;
    return val;
}

struct Value *new_vfuncall(Symbol funname, Values *args, bool is_builtin)
{
    enum ValueType t = is_builtin ? VTYPE_BUILTIN_FUNCALL: VTYPE_FUNCALL;
    struct Value *val = new_value(t, TYPE_UNDEFINED);
    val->funcall = new_funcall(funname, args);
    return val;
}

struct Value *new_constructor(Symbol funname, Values *args, bool is_builtin)
{
    // Constructor name should be same as classname
    enum ValueType t = is_builtin ? VTYPE_BUILTIN_CONSTRUCTOR: VTYPE_CONSTRUCTOR;
    struct Value *val = new_value(t, funname);
    val->funcall = new_funcall(funname, args);
    return val;
}

struct Value *new_get(Symbol symbol, LValues *lvalues)
{
    struct Value *val = new_value(VTYPE_GET, TYPE_UNDEFINED);
    val->get = new_accessor(symbol, lvalues);
    return val;
}

struct Value *new_expr(struct Expr *expr)
{
    struct Value *val = new_value(VTYPE_EXPR, TYPE_UNDEFINED);
    val->expr = expr;
    return val;
}

struct LValue *new_property(Symbol property)
{
    struct LValue *lvalue = allocator_malloc(sizeof(struct LValue));
    lvalue->type = LV_PROPERTY;
    lvalue->type = TYPE_UNDEFINED;
    lvalue->property = property;
    return lvalue;
}

struct LValue *new_index(struct Value *index)
{
    struct LValue *lvalue = allocator_malloc(sizeof(struct LValue));
    lvalue->type = LV_INDEX;
    lvalue->type = TYPE_UNDEFINED;
    lvalue->index = index;
    return lvalue;
}

/* Macros used to generate functions for creating Expressions.
 * Unary and binary operators have the same structure, so this 
 * avoids us having to copy-paste the same snippets many times */

#define define_unary_op(name, operator) \
struct Expr *name(struct Value *val1) {\
    struct Expr *expr = allocator_malloc(sizeof(struct Expr));\
    expr->op = operator;\
    expr->val1 = val1;\
    expr->val2 = NULL;\
    return expr;\
}

define_unary_op(unary_plus, OP_UNARY_PLUS)
define_unary_op(unary_minus, OP_UNARY_MINUS)

#undef define_unary_op

#define define_binary_op(name, operator) \
struct Expr *name(struct Value *val1, struct Value *val2) {\
    struct Expr *expr = allocator_malloc(sizeof(struct Expr));\
    expr->op = operator;\
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
define_binary_op(bin_percent, OP_PERCENT)

#undef define_binary_op

