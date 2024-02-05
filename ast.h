#ifndef AST_H
#define AST_H
#include "common.h"

/* Misc forward declarations */
struct Expr;
struct Value;
struct Statement;

typedef struct List TopLevelDecls;
typedef struct List Values;
typedef struct List Statements;
typedef struct List Dim;
typedef struct List Definitions;

enum ValueType {
    VTYPE_SYMBOL,
    VTYPE_STRING,
    VTYPE_INT,
    VTYPE_FLOAT,
    VTYPE_BOOL,
    VTYPE_EXPR,
    VTYPE_FUNCALL
};

enum OperatorType {
    OP_OR,
    OP_AND,
    OP_EQEQ,
    OP_NOT_EQ,
    OP_GREATER_EQ,
    OP_GREATER,
    OP_LESS_EQ,
    OP_LESS,
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_UNARY_PLUS,
    OP_UNARY_MINUS
};

enum Type {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING
};

enum TLDType {
    TLD_TYPE_CLASS,
    TLD_TYPE_FUNDEF
};

struct Class {
    Symbol name;
    Definitions *definitions;
};

struct FunDef {
    Symbol name;
    Definitions *definitions;
    Statements *stmts;
};

struct TopLevelDecl {
    enum TLDType type;
    struct Class *cls;
    struct FunDef *fundef;
};

struct FunCall {
    Symbol funname;
    Values *args;
};

struct Expr {
    enum OperatorType op;
    struct Value *val1;
    struct Value *val2;
};

struct Value {
    enum ValueType type;
    union {
        char *string;
        Symbol symbol;
        long integer;
        double floating;
        long boolean;
        struct Expr *expr;
        struct FunCall *funcall;
    };
};

struct Definition {
    Symbol name;
    enum Type type;
};

struct Set {
    Symbol symbol;
    struct Value *val;
};

struct IfStatement {
    struct Value *condition;
    Statements *if_stmts;
    Statements *else_stmts;
};

struct While {
    struct Value *condition;
    Statements *stmts;
};

struct For {
    struct Set *start;
    struct Value *stop;
    Statements *stmts;
};

struct ForEach {
    Symbol symbol;
    struct Value *condition;
    Statements *stmts;
};

enum StatementType {
    STMT_DIM,
    STMT_SET,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FOREACH,
    STMT_FUNCTION_DEF,
    STMT_EXIT_FOR,
    STMT_EXIT_WHILE,
    STMT_EXIT_FUNCTION,
    STMT_FUNCALL
};

struct Statement {
    enum StatementType type;
    union {
        Dim *dim;
        struct Set *set;
        struct IfStatement *if_stmt;
        struct While *while_stmt;
        struct For *for_stmt;
        struct ForEach *for_each;
        struct FunCall *funcall;
    };
};

/* TLD constructors */
struct TopLevelDecl *new_class(Symbol symbol, Definitions *definitions);
struct TopLevelDecl *new_fundef(Symbol symbol, Definitions *definitions, Statements *stmts);

/* Statement constructors */
struct Statement *new_set(Symbol symbol, struct Value *val);
struct Statement *new_if(struct Value *condition, Statements *if_stmts, Statements *else_stmts);
struct Statement *new_while(struct Value *condition, Statements *stmts);
struct Statement *new_dim(Dim *dim);
struct Definition *new_define(Symbol name, enum Type type);
struct Statement *new_sfuncall(Symbol funname, Values *args);

/* Value constructors */
struct Value *new_string(char *string);
struct Value *new_symbol(uint64_t symbol);
struct Value *new_integer(long integer);
struct Value *new_floating(double floating);
struct Value *new_boolean(int boolean);
struct Value *new_vfuncall(Symbol funname, Values *args);
struct Value *new_expr(struct Expr *expr);

/* Expression constructors */

/* Unary */
struct Expr *unary_plus(struct Value *val1);
struct Expr *unary_minus(struct Value *val1);

/* Binary */
struct Expr *name(struct Value *val1, struct Value *val2);
struct Expr *bin_or(struct Value *val1, struct Value *val2);
struct Expr *bin_and(struct Value *val1, struct Value *val2);
struct Expr *bin_eqeq(struct Value *val1, struct Value *val2);
struct Expr *bin_not_eq(struct Value *val1, struct Value *val2);
struct Expr *bin_greater_eq(struct Value *val1, struct Value *val2);
struct Expr *bin_greater(struct Value *val1, struct Value *val2);
struct Expr *bin_less_eq(struct Value *val1, struct Value *val2);
struct Expr *bin_less(struct Value *val1, struct Value *val2);
struct Expr *bin_plus(struct Value *val1, struct Value *val2);
struct Expr *bin_minus(struct Value *val1, struct Value *val2);
struct Expr *bin_star(struct Value *val1, struct Value *val2);
struct Expr *bin_slash(struct Value *val1, struct Value *val2);

#endif

