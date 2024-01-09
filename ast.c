#include "common.h"
#include "ast.h"

/* List functions */
struct List *new_list(void *value)
{
    struct List *list = malloc(sizeof(struct List));
    list->value = value;
    list->next = NULL;
    list->prev = NULL;
    return list;
}

struct List *append(struct List *list, void *value)
{
    list->prev = new_list(value);
    list->prev->next = list;
    return list->prev;
}

/* Our parser walks the list while building it, so we want to move the
 * pointer to the start of the list when we're finished. */
struct List *reset_list_head(struct List *list)
{
    while (list->prev != NULL) list = list->prev;
    return list;
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
    printf("conna grash here\n");
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_IF;
    stmt->if_stmt = malloc(sizeof(struct IfStatement));
    stmt->if_stmt->condition = condition;
    stmt->if_stmt->if_stmts = reset_list_head(if_stmts);
    if (stmt->if_stmt->else_stmts) {
        stmt->if_stmt->else_stmts = reset_list_head(else_stmts);
    }
    printf("maybe bed time\n");
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

struct Statement *new_dim(Dim *dim)
{
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_DIM;
    stmt->dim = reset_list_head(dim);
    return stmt;
}
 
struct Definition *new_define(Symbol name, enum Type type)
{
    struct Definition *def = malloc(sizeof(struct Definition));
    def->name = name;
    def->type = type;
    return def;
}

/* Functions for creating Values */
struct Value *new_string(char *string)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_STRING;
    val->string = string;
    return val;
}

struct Value *new_symbol(char *symbol)
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

struct Value *new_funcall(struct FunCall *funcall)
{
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_FUNCALL;
    val->funcall = funcall;
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


/* Printers. Currently used for debugging purposes but could be used for 
 * building a formatter in the future */
void print_expr(struct Expr *expr)
{
    printf("(");
    switch (expr->op) {
    case OP_OR:
        print_value(expr->val1);
        printf(" or ");
        print_value(expr->val2);
        break;
    case OP_AND:
        print_value(expr->val1);
        printf(" and ");
        print_value(expr->val2);
        break;
    case OP_EQEQ:
        print_value(expr->val1);
        printf(" == ");
        print_value(expr->val2);
        break;
    case OP_NOT_EQ:
        print_value(expr->val1);
        printf(" <> ");
        print_value(expr->val2);
        break;
    case OP_GREATER_EQ:
        print_value(expr->val1);
        printf(" >= ");
        print_value(expr->val2);
        break;
    case OP_GREATER:
        print_value(expr->val1);
        printf(" > ");
        print_value(expr->val2);
        break;
    case OP_LESS_EQ:
        print_value(expr->val1);
        printf(" <= ");
        print_value(expr->val2);
        break;
    case OP_LESS:
        print_value(expr->val1);
        printf(" < ");
        print_value(expr->val2);
        break;
    case OP_PLUS:
        print_value(expr->val1);
        printf(" + ");
        print_value(expr->val2);
        break;
    case OP_MINUS:
        print_value(expr->val1);
        printf(" - ");
        print_value(expr->val2);
        break;
    case OP_STAR:
        print_value(expr->val1);
        printf(" * ");
        print_value(expr->val2);
        break;
    case OP_SLASH:
        print_value(expr->val1);
        printf(" / ");
        print_value(expr->val2);
        break;
    case OP_UNARY_PLUS:
        printf("+");
        print_value(expr->val1);
        break;
    case OP_UNARY_MINUS:
        printf("-");
        print_value(expr->val1);
        break;
    }
    printf(")");
}

void print_value(struct Value *val)
{
    switch (val->type) {
    case VTYPE_SYMBOL:
        printf("%s", val->symbol);
        break;
    case VTYPE_STRING:
        printf("%s", val->string);
        break;
    case VTYPE_INT:
        printf("%ld", val->integer);
        break;
    case VTYPE_FLOAT:
        printf("%f", val->floating);
        break;
    case VTYPE_BOOL:
        printf("%d", val->boolean);
        break;
    case VTYPE_EXPR:
        print_expr(val->expr);
        break;
    case VTYPE_FUNCALL:
        printf("not yet implemented");
        break;
    }
}

static void print_statements_indent(Statements *stmts, int indent);

/* :) */
static void left_pad(int indent) {
    for (int i = 0; i < indent; i++) putchar(' ');
}

static const int TAB_WIDTH = 4;

static void print_statement_indent(struct Statement *stmt, int indent)
{
    Dim *dim = NULL;
    struct Definition *def = NULL;
    left_pad(indent);
    switch (stmt->type) {
    case STMT_DIM:
        dim = stmt->dim;
        printf("dim ");
        while (dim != NULL) {
            def = (struct Definition *) dim->value;
            printf("%s as ", def->name);
            switch (def->type) {
                case TYPE_INT:
                    printf("int");
                    break;
                case TYPE_FLOAT:
                    printf("float");
                    break;
                case TYPE_BOOL:
                    printf("bool");
                    break;
                case TYPE_STRING:
                    printf("string");
                    break;
            }
            if (dim->next) printf(", ");
            dim = dim->next;
        }
        break;
    case STMT_SET:
        printf("%s = ", stmt->set->symbol);
        print_value(stmt->set->val);
        break;
    case STMT_IF:
        printf("if ");
        print_value(stmt->if_stmt->condition);
        printf(" then\n");
        print_statements_indent(stmt->if_stmt->if_stmts, indent + TAB_WIDTH);
        if (stmt->if_stmt->else_stmts) {
            left_pad(indent);
            printf("else\n");
            print_statements_indent(stmt->if_stmt->else_stmts, indent + TAB_WIDTH);
        }
        printf("end if");
        break;
    case STMT_WHILE:
        break;
    case STMT_FOR:
        break;
    case STMT_FOREACH:
        break;
    case STMT_FUNCTION_DEF:
        break;
    case STMT_EXIT_WHILE:
        break;
    case STMT_EXIT_FOR:
        break;
    case STMT_EXIT_FUNCTION:
        break;
    }
    printf("\n");
}

static void print_statements_indent(Statements *stmts, int indent)
{
    while (stmts != NULL)
    {
        print_statement_indent(stmts->value, indent);
        stmts = stmts->next;
    }
}

void print_statement(struct Statement *stmt)
{
    print_statement_indent(stmt, 0);
}

void print_statements(Statements *stmts)
{
    print_statements_indent(stmts, 0);
}

