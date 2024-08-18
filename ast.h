#ifndef AST_H
#define AST_H
#include "common.h"

/* Misc forward declarations */
struct Expr;
struct Value;
struct Statement;

/* Typedefs for void* lists, to keep track of their contents */
typedef struct LinkedList TopLevelDecls;
typedef struct LinkedList Values;
typedef struct LinkedList Statements;
typedef struct LinkedList Definitions;
typedef struct LinkedList Methods;
typedef struct LinkedList LValues;

enum ValueType {
    // VTYPE_SYMBOL,
    VTYPE_STRING,
    VTYPE_INT,
    VTYPE_FLOAT,
    VTYPE_BOOL,
    VTYPE_EXPR,
    VTYPE_FUNCALL,
    VTYPE_CONSTRUCTOR,
    VTYPE_GET
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
    OP_PERCENT,
    OP_UNARY_PLUS,
    OP_UNARY_MINUS
};

enum TLDType {
    TLD_TYPE_CLASS,
    TLD_TYPE_FUNDEF
};

struct Class {
    Symbol name; // Name is same as type
    Definitions *definitions;
    Methods *methods;
};

struct FunDef {
    Symbol name;
    Type rettype;
    Definitions *args;
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

enum LValueType {
    LV_PROPERTY,
    LV_INDEX
};

struct LValue {
    enum LValueType lvtype;
    Type type;
    union {
        Symbol property;
        struct Value *index;
    };
};

struct Definition {
    size_t scope_offset;
    Symbol name;
    Type type;
};

struct Accessor {
    struct Definition *definition;
    LValues *lvalues;
};

struct Value {
    enum ValueType vtype;
    Type type;
    union {
        char *string;
        // Symbol string;
        // Symbol symbol; // "get" has replaced this
        long integer;
        double floating;
        long boolean;
        struct Expr *expr;
        struct FunCall *funcall;
        struct FunCall *constructor;
        struct Accessor *get;
    };
};

struct Set {
    // Symbol symbol;
    // Type type;
    // LValues *lvalues; // May be null
    bool is_define;
    struct Accessor *to_set;
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
    struct Statement *init;
    struct Value *condition;
    struct Statement *increment;
    Statements *stmts;
};

struct ForEach {
    Symbol symbol;
    struct Value *condition;
    Statements *stmts;
};

enum StatementType {
    STMT_LET,
    STMT_SET,
    STMT_GET,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FOREACH,
    STMT_FUNCALL,
    STMT_RETURN
};

struct Statement {
    enum StatementType type;
    union {
        Definitions *let;
        struct Set *set;
        struct IfStatement *if_stmt;
        struct While *while_stmt;
        struct For *for_stmt;
        struct ForEach *for_each;
        struct FunCall *funcall;
        struct Value *ret;
    };
};

/* Misc. helper functions */
struct Definition *lookup_property(struct Class *cls, Symbol name);
uint64_t lookup_property_index(struct Class *cls, Symbol name);

/* TLD constructors */
struct TopLevelDecl *new_class(Symbol symbol, Definitions *definitions, Methods *methods);
struct TopLevelDecl *new_tld_fundef(Symbol symbol, Type rettype, Definitions *args,
        Statements *stmts);
struct FunDef *new_fundef(Symbol symbol, Type rettype, Definitions *args, Statements *stmts);

/* Statement constructors */
struct Statement *new_set(Symbol symbol, struct Value *val, LValues *lvalues, bool is_define);
struct Statement *new_if(struct Value *condition, Statements *if_stmts);
struct Statement *add_elseif(struct IfStatement *if_stmt, struct Statement *elseif_stmt);
void add_else(struct IfStatement *if_stmt, Statements *else_stmts);
struct Statement *new_while(struct Value *condition, Statements *stmts);
struct Statement *new_for(struct Statement *init, struct Value *condition,
        struct Statement *increment, Statements *stmts);
struct Statement *new_let(Definitions *let);
struct Definition *new_define(Symbol name, Type type);
struct Statement *new_sfuncall(Symbol funname, Values *args);
struct Statement *new_return(struct Value *val);

/* Value constructors */
struct Value *new_string(char *string);
// struct Value *new_symbol(uint64_t symbol);
struct Value *new_integer(long integer);
struct Value *new_floating(double floating);
struct Value *new_boolean(int boolean);
struct Value *new_vfuncall(Symbol funname, Values *args);
struct Value *new_constructor(Symbol funname, Values *args);
struct Value *new_get(Symbol symbol, LValues *lvalues);
struct Value *new_expr(struct Expr *expr);
struct LValue *new_property(Symbol property);
struct LValue *new_index(struct Value *index);

/* Expression constructors */

/* Unary */
struct Expr *unary_plus(struct Value *val1);
struct Expr *unary_minus(struct Value *val1);

/* Binary */
#define bin_decl(name) struct Expr *name(struct Value *val1, struct Value *val2);
bin_decl(bin_or)
bin_decl(bin_and)
bin_decl(bin_eqeq)
bin_decl(bin_not_eq)
bin_decl(bin_greater_eq)
bin_decl(bin_greater)
bin_decl(bin_less_eq)
bin_decl(bin_less)
bin_decl(bin_plus)
bin_decl(bin_minus)
bin_decl(bin_star)
bin_decl(bin_slash)
bin_decl(bin_percent)
#undef bin_decl

#endif

