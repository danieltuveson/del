#include "common.h"
#include "allocator.h"
// #include "printers.h"
#include "linkedlist.h"
#include "del.h"
#include "ffi.h"
#include "ast.h"

/* Functions to create top level definitions */
static struct TopLevelDecl *new_tld(struct Globals *globals, enum TLDType tld_type)
{
    struct TopLevelDecl *tld = allocator_malloc(globals->allocator, sizeof(struct TopLevelDecl));
    tld->type = tld_type;
    return tld;
}

struct TopLevelDecl *new_class(struct Globals *globals, Symbol symbol,
        Definitions *definitions, TopLevelDecls *methods)
{
    globals->class_count++;
    struct TopLevelDecl *tld = new_tld(globals, TLD_TYPE_CLASS);
    tld->cls = allocator_malloc(globals->allocator, sizeof(struct Class));
    tld->cls->name = symbol;
    tld->cls->definitions = definitions;
    tld->cls->methods = methods;
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

struct FunDef *lookup_method(struct Class *cls, Symbol name)
{
    linkedlist_foreach(lnode, cls->methods->head) {
        struct FunDef *method = ((struct TopLevelDecl *)lnode->value)->fundef;
        if (method->name == name) {
            return method;
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

static struct GetProperty *new_get_property_access(struct Globals *globals, struct Value *accessor,
        Symbol property)
{
    struct GetProperty *get = allocator_malloc(globals->allocator, sizeof(struct GetProperty));
    get->accessor = accessor;
    get->property = property;
    return get;
}

static struct GetProperty *new_get_indexed_access(struct Globals *globals, struct Value *accessor,
        struct Value *index)
{
    struct GetProperty *get = allocator_malloc(globals->allocator, sizeof(struct GetProperty));
    get->accessor = accessor;
    get->index = index;
    return get;
}

struct Accessor *new_accessor(struct Globals *globals, Symbol symbol, LValues *lvalues)
{
    struct Accessor *accessor = allocator_malloc(globals->allocator, sizeof(struct Accessor));
    accessor->definition = new_define(globals, symbol, TYPE_UNDEFINED);
    accessor->lvalues = lvalues;
    return accessor;
}

struct TopLevelDecl *new_tld_fundef(struct Globals *globals, Symbol symbol, Type rettype,
        Definitions *args, Statements *stmts)
{
    globals->function_count++;
    struct TopLevelDecl *tld = new_tld(globals, TLD_TYPE_FUNDEF);
    struct FunDef *fundef = allocator_malloc(globals->allocator, sizeof(*fundef));
    fundef->name = symbol;
    fundef->is_foreign = false;
    fundef->rettype = rettype;
    fundef->num_locals = 0;
    fundef->args = args;
    fundef->stmts = stmts;
    tld->fundef = fundef;
    return tld;
}

struct TopLevelDecl *new_tld_foreign_fundef(struct Globals *globals, Symbol symbol, Type rettype,
        Types *types, struct ForeignFunctionBody *ffb)
{
    globals->function_count++;
    struct TopLevelDecl *tld = new_tld(globals, TLD_TYPE_FUNDEF);
    struct FunDef *fundef = allocator_malloc(globals->allocator, sizeof(*fundef));
    fundef->name = symbol;
    fundef->is_foreign = true;
    fundef->rettype = rettype;
    fundef->num_locals = 0;
    fundef->types = types; 
    fundef->ffb = ffb;
    tld->fundef = fundef;
    return tld;
}

/* Functions to create Statements */
struct Statement *new_stmt(struct Globals *globals, enum StatementType st)
{
    struct Statement *stmt = allocator_malloc(globals->allocator, sizeof(*stmt));
    stmt->type = st;
    return stmt;
}

struct Statement *new_set_local(struct Globals *globals, Symbol variable, struct Value *val,
        bool is_define)
{
    struct Statement *stmt = new_stmt(globals, STMT_SET_LOCAL);
    stmt->set_local = allocator_malloc(globals->allocator, sizeof(*(stmt->set_local)));
    stmt->set_local->is_define = is_define;
    stmt->set_local->def = new_define(globals, variable, TYPE_UNDEFINED);
    stmt->set_local->expr = val;
    return stmt;
}

struct Statement *new_set_property(struct Globals *globals, struct Value *accessor, Symbol property,
        struct Value *val)
{
    struct Statement *stmt = new_stmt(globals, STMT_SET_PROPERTY);
    stmt->set_property = allocator_malloc(globals->allocator, sizeof(*(stmt->set_property)));
    stmt->set_property->access = new_get_property_access(globals, accessor, property);
    stmt->set_property->expr = val;
    return stmt;
}

struct Statement *new_set_indexed(struct Globals *globals, struct Value *accessor,
        struct Value *index, struct Value *val)
{
    struct Statement *stmt = new_stmt(globals, STMT_SET_INDEX);
    stmt->set_property = allocator_malloc(globals->allocator, sizeof(*(stmt->set_property)));
    stmt->set_property->access = new_get_indexed_access(globals, accessor, index);
    stmt->set_property->expr = val;
    return stmt;
}

struct Statement *new_if(struct Globals *globals, struct Value *condition, Statements *if_stmts)
{
    struct Statement *stmt = new_stmt(globals, STMT_IF);
    stmt->if_stmt = allocator_malloc(globals->allocator, sizeof(struct IfStatement));
    stmt->if_stmt->condition = condition;
    stmt->if_stmt->if_stmts = if_stmts;
    stmt->if_stmt->else_stmts = NULL;
    return stmt;
}

// This is a little hacky, maybe re-write the IfStatement structure to accomodate else-ifs
struct Statement *add_elseif(struct Globals *globals, struct IfStatement *if_stmt, struct Statement *elseif_stmt)
{
    if_stmt->else_stmts = linkedlist_new(globals->allocator);
    linkedlist_append(if_stmt->else_stmts, elseif_stmt);
    return if_stmt->else_stmts->head->value;
}

void add_else(struct IfStatement *if_stmt, Statements *else_stmts)
{
    if_stmt->else_stmts = else_stmts;
}

struct Statement *new_while(struct Globals *globals, struct Value *condition, Statements *stmts)
{
    struct Statement *stmt = new_stmt(globals, STMT_WHILE);
    stmt->while_stmt = allocator_malloc(globals->allocator, sizeof(struct While));
    stmt->while_stmt->condition = condition;
    stmt->while_stmt->stmts = stmts;
    return stmt;
}

struct Statement *new_for(struct Globals *globals, struct Statement *init, struct Value *condition,
        struct Statement *increment, Statements *stmts)
{
    struct Statement *stmt = new_stmt(globals, STMT_FOR);
    stmt->for_stmt = allocator_malloc(globals->allocator, sizeof(struct For));
    stmt->for_stmt->init = init;
    stmt->for_stmt->condition = condition;
    stmt->for_stmt->increment = increment;
    stmt->for_stmt->stmts = stmts;
    return stmt;
}

struct Statement *new_let(struct Globals *globals, Definitions *let)
{
    struct Statement *stmt = new_stmt(globals, STMT_LET);
    stmt->let = let;
    return stmt;
}
 
struct Definition *new_define(struct Globals *globals, Symbol name, Type type)
{
    struct Definition *def = allocator_malloc(globals->allocator, sizeof(struct Definition));
    def->name = name;
    def->type = type;
    def->scope_offset = 0;
    return def;
}

static struct FunCall *new_funcall(struct Globals *globals, struct Accessor *access,
        Values *args)
{
    struct FunCall *funcall = allocator_malloc(globals->allocator, sizeof(struct FunCall));
    funcall->access = access;
    funcall->args = args;
    return funcall;
}

struct Statement *new_sfuncall(struct Globals *globals, struct Accessor *access, Values *args,
        bool is_builtin)
{
    enum StatementType t = is_builtin ? STMT_BUILTIN_FUNCALL : STMT_FUNCALL;
    struct Statement *stmt = new_stmt(globals, t);
    stmt->funcall = new_funcall(globals, access, args);
    return stmt;
}

struct Statement *new_return(struct Globals *globals, struct Value *val)
{
    struct Statement *stmt = new_stmt(globals, STMT_RETURN);
    stmt->val = val;
    return stmt;
}
struct Statement *new_break(struct Globals *globals)
{
    struct Statement *stmt = new_stmt(globals, STMT_BREAK);
    stmt->val = NULL;
    return stmt;
}

struct Statement *new_continue(struct Globals *globals)
{
    struct Statement *stmt = new_stmt(globals, STMT_CONTINUE);
    stmt->val = NULL;
    return stmt;
}

struct Statement *new_increment(struct Globals *globals, struct Value *val)
{
    struct Statement *stmt = new_stmt(globals, STMT_INC);
    stmt->val = val;
    return stmt;
}

struct Statement *new_decrement(struct Globals *globals, struct Value *val)
{
    struct Statement *stmt = new_stmt(globals, STMT_DEC);
    stmt->val = val;
    return stmt;
}

/* Functions for creating Values */
static struct Value *new_value(struct Globals *globals, enum ValueType vtype, Type type)
{
    struct Value *val = allocator_malloc(globals->allocator, sizeof(struct Value));
    val->vtype = vtype;
    val->type = type;
    return val;
}

struct Value *new_string(struct Globals *globals, char *string)
{
    globals->string_count++;
    struct Value *val = new_value(globals, VTYPE_STRING, TYPE_STRING);
    val->string = string;
    return val;
}

struct Value *new_byte(struct Globals *globals, char byte)
{
    struct Value *val = new_value(globals, VTYPE_BYTE, TYPE_BYTE);
    val->byte = byte;
    return val;
}

struct Value *new_integer(struct Globals *globals, int64_t integer)
{
    struct Value *val = new_value(globals, VTYPE_INT, TYPE_INT);
    val->integer = integer;
    return val;
}

struct Value *new_floating(struct Globals *globals, double floating)
{
    struct Value *val = new_value(globals, VTYPE_FLOAT, TYPE_FLOAT);
    val->floating = floating;
    return val;
}

struct Value *new_boolean(struct Globals *globals, int64_t boolean)
{
    struct Value *val = new_value(globals, VTYPE_BOOL, TYPE_BOOL);
    val->boolean = boolean;
    return val;
}

struct Value *new_null(struct Globals *globals)
{
    struct Value *val = new_value(globals, VTYPE_NULL, TYPE_NULL);
    val->integer = 0;
    return val;
}

struct Value *new_vfuncall(struct Globals *globals, struct Accessor *access, Values *args,
        bool is_builtin)
{
    enum ValueType t = is_builtin ? VTYPE_BUILTIN_FUNCALL: VTYPE_FUNCALL;
    struct Value *val = new_value(globals, t, TYPE_UNDEFINED);
    val->funcall = new_funcall(globals, access, args);
    return val;
}

struct Value *new_constructor(struct Globals *globals, struct Accessor *access,
        Types *types, Values *args, bool is_builtin)
{
    // Constructor name should be same as classname
    enum ValueType t = is_builtin ? VTYPE_BUILTIN_CONSTRUCTOR : VTYPE_CONSTRUCTOR;
    struct Value *val = new_value(globals, t, access->definition->name);
    val->constructor = allocator_malloc(globals->allocator, sizeof(struct Constructor));
    val->constructor->types = types;
    val->constructor->funcall = new_funcall(globals, access, args);
    return val;
}

struct Value *new_get_local(struct Globals *globals, Symbol variable)
{
    struct Value *val = new_value(globals, VTYPE_GET_LOCAL, TYPE_UNDEFINED);
    val->get_local = new_define(globals, variable, TYPE_UNDEFINED);
    return val;
}

struct Value *new_get_property(struct Globals *globals, struct Value *accessor, Symbol property)
{
    struct Value *val = new_value(globals, VTYPE_GET_PROPERTY, TYPE_UNDEFINED);
    val->get_property = new_get_property_access(globals, accessor, property);
    return val;
}

struct Value *new_get_indexed(struct Globals *globals, struct Value *accessor,
        struct Value *index)
{
    struct Value *val = new_value(globals, VTYPE_INDEX, TYPE_UNDEFINED);
    val->get_property = new_get_indexed_access(globals, accessor, index);
    return val;
}

struct Value *new_cast(struct Globals *globals, struct Value *value, Type type)
{
    struct Value *val = new_value(globals, VTYPE_CAST, TYPE_UNDEFINED);
    val->cast = allocator_malloc(globals->allocator, sizeof(struct Cast));
    val->cast->value = value;
    val->cast->type = type;
    return val;
}

struct Value *new_expr(struct Globals *globals, struct Expr *expr)
{
    struct Value *val = new_value(globals, VTYPE_EXPR, TYPE_UNDEFINED);
    val->expr = expr;
    return val;
}

struct Value *new_array(struct Globals *globals, Values *vals)
{
    struct Value *val = new_value(globals, VTYPE_ARRAY_LITERAL, TYPE_UNDEFINED);
    val->array = vals;
    return val;
}

/* Macros used to generate functions for creating Expressions.
 * Unary and binary operators have the same structure, so this 
 * avoids us having to copy-paste the same snippets many times */

#define define_unary_op(name, operator) \
struct Expr *name(struct Globals *globals, struct Value *val1) {\
    struct Expr *expr = allocator_malloc(globals->allocator, sizeof(struct Expr));\
    expr->op = operator;\
    expr->val1 = val1;\
    expr->val2 = NULL;\
    return expr;\
}

define_unary_op(unary_plus, OP_UNARY_PLUS)
define_unary_op(unary_minus, OP_UNARY_MINUS)
define_unary_op(unary_not, OP_UNARY_NOT)

#undef define_unary_op

#define define_binary_op(name, operator) \
struct Expr *name(struct Globals *globals, struct Value *val1, struct Value *val2) {\
    struct Expr *expr = allocator_malloc(globals->allocator, sizeof(struct Expr));\
    expr->op = operator;\
    expr->val1 = val1;\
    expr->val2 = val2;\
    return expr;\
}

// define_binary_op(bin_or, OP_GET)
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

