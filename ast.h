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
typedef struct LinkedList Types;
typedef struct LinkedList Statements;
typedef struct LinkedList Definitions;
typedef struct LinkedList LValues;

enum ValueType {
    VTYPE_STRING,
    VTYPE_INT,
    VTYPE_FLOAT,
    VTYPE_BOOL,
    VTYPE_NULL,
    VTYPE_EXPR,
    VTYPE_FUNCALL,
    VTYPE_BUILTIN_FUNCALL,
    VTYPE_CONSTRUCTOR,
    VTYPE_BUILTIN_CONSTRUCTOR,
    VTYPE_GET_LOCAL,
    VTYPE_GET_PROPERTY,
    VTYPE_INDEX
};

enum OperatorType {
    // Binary ops
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
    // Unary ops
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
    TopLevelDecls *methods;
};

struct FunDef {
    Symbol name;
    Type rettype;
    uint64_t num_locals; // I don't know if I have a purpose for this
    Definitions *args;
    Statements *stmts;
};

struct TopLevelDecl {
    enum TLDType type;
    union {
        struct Class *cls;
        struct FunDef *fundef;
    };
};

struct FunCall {
    struct Accessor *access;
    Values *args;
};

struct Constructor {
    Types *types;
    struct FunCall *funcall;
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
        long integer;
        double floating;
        long boolean;
        struct Expr *expr;
        struct FunCall *funcall;
        struct Constructor *constructor;
        struct Accessor *get;
        struct MethodCall *method_call;
        // New values to replace get
        struct Definition *get_local;
        struct GetProperty *get_property;
    };
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
    STMT_SET_LOCAL,
    STMT_SET_PROPERTY,
    STMT_SET_INDEX,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FOREACH,
    STMT_FUNCALL,
    STMT_BUILTIN_FUNCALL,
    STMT_RETURN,
    STMT_INC,
    STMT_DEC
};

struct GetProperty {
    struct Value *accessor;
    union {
        Symbol property;
        struct Value *index;
    };
};

struct SetLocal {
    bool is_define;
    struct Definition *def;
    struct Value *expr;
};

struct SetProperty {
    struct GetProperty *access;
    struct Value *expr;
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
        struct Value *val;
        // Need to replace old sets with these
        struct SetLocal *set_local;
        struct SetProperty *set_property;
    };
};

/* Misc. helper functions */
struct Definition *lookup_property(struct Class *cls, Symbol name);
struct FunDef *lookup_method(struct Class *cls, Symbol name);
uint64_t lookup_property_index(struct Class *cls, Symbol name);
struct Accessor *new_accessor(struct Globals *globals, Symbol symbol, LValues *lvalues);

/* TLD constructors */
struct TopLevelDecl *new_class(struct Globals *globals, Symbol symbol, Definitions *definitions,
        TopLevelDecls *methods);
struct TopLevelDecl *new_tld_fundef(struct Globals *globals, Symbol symbol, Type rettype,
        Definitions *args, Statements *stmts);
struct FunDef *new_fundef(struct Globals *globals, Symbol symbol, Type rettype,
        Definitions *args, Statements *stmts);

/* Statement constructors */
struct Statement *new_stmt(struct Globals *globals, enum StatementType st);
struct Statement *new_set_local(struct Globals *globals, Symbol variable, struct Value *val,
        bool is_define);
struct Statement *new_set_property(struct Globals *globals, struct Value *accessor, Symbol property,
        struct Value *val);
struct Statement *new_set_indexed(struct Globals *globals, struct Value *accessor,
        struct Value *index, struct Value *val);
struct Statement *new_if(struct Globals *globals, struct Value *condition, Statements *if_stmts);
struct Statement *add_elseif(struct Globals *globals, struct IfStatement *if_stmt,
        struct Statement *elseif_stmt);
void add_else(struct IfStatement *if_stmt, Statements *else_stmts);
struct Statement *new_while(struct Globals *globals, struct Value *condition, Statements *stmts);
struct Statement *new_for(struct Globals *globals, struct Statement *init,
        struct Value *condition, struct Statement *increment, Statements *stmts);
struct Statement *new_let(struct Globals *globals, Definitions *let);
struct Definition *new_define(struct Globals *globals, Symbol name, Type type);
struct Statement *new_sfuncall(struct Globals *globals, struct Accessor *access, Values *args,
        bool is_builtin);
struct Statement *new_return(struct Globals *globals, struct Value *val);
struct Statement *new_increment(struct Globals *globals, struct Value *val);
struct Statement *new_decrement(struct Globals *globals, struct Value *val);

/* Value constructors */
struct Value *new_string(struct Globals *globals, char *string);
struct Value *new_integer(struct Globals *globals, long integer);
struct Value *new_floating(struct Globals *globals, double floating);
struct Value *new_boolean(struct Globals *globals, int boolean);
struct Value *new_null(struct Globals *globals);
struct Value *new_vfuncall(struct Globals *globals, struct Accessor *access, Values *args,
        bool is_builtin);
struct Value *new_constructor(struct Globals *globals, struct Accessor *access,
        Types *types, Values *args, bool is_builtin);
// struct Value *new_get(struct Globals *globals, Symbol symbol, LValues *lvalues);
struct Value *new_expr(struct Globals *globals, struct Expr *expr);
struct Value *new_get_local(struct Globals *globals, Symbol variable);
struct Value *new_get_property(struct Globals *globals, struct Value *accessor, Symbol property);
struct Value *new_get_indexed(struct Globals *globals, struct Value *accessor,
        struct Value *index);

struct LValue *new_property(struct Globals *globals, Symbol property);
struct LValue *new_index(struct Globals *globals, struct Value *index);

/* Expression constructors */

/* Unary */
struct Expr *unary_plus(struct Globals *globals, struct Value *val1);
struct Expr *unary_minus(struct Globals *globals, struct Value *val1);

/* Binary */
#define bin_decl(name)\
    struct Expr *name(struct Globals *globals, struct Value *val1, struct Value *val2)
bin_decl(bin_or);
bin_decl(bin_and);
bin_decl(bin_eqeq);
bin_decl(bin_not_eq);
bin_decl(bin_greater_eq);
bin_decl(bin_greater);
bin_decl(bin_less_eq);
bin_decl(bin_less);
bin_decl(bin_plus);
bin_decl(bin_minus);
bin_decl(bin_star);
bin_decl(bin_slash);
bin_decl(bin_percent);
#undef bin_decl

#endif

