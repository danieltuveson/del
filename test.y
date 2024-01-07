%{
#include "test.h"
#define YYDEBUG 1

int yylex();
int yyerror(char *s);

struct List;
struct List {
    void *value;
    struct List *prev;
    struct List *next;
};

struct List *new_list(void *value) {
    struct List *list = malloc(sizeof(struct List));
    list->value = value;
    list->next = NULL;
    list->prev = NULL;
    return list;
}

struct List *push(struct List *list, void *value) {
    list->next = new_list(value);
    list->next->prev = list;
    return list->next;
}

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

struct Expr;
struct Value;

struct FunCall {
    char *funname;
    struct Value **values;
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
        int boolean;
        struct Expr *expr;
        struct FunCall *funcall;
    };
};

struct Statement;

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
    STMT_EXIT_FUNCTION
};

struct Statement {
    enum StatementType type;
    union {
        struct Set *set;
        struct IfStatement *if_stmt;
        struct While *while_stmt;
        struct For *for_stmt;
        struct ForEach *for_each;
    };
};

struct Statement *new_set(Symbol symbol, struct Value *val) {
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
    stmt->if_stmt->if_stmts = if_stmts;
    stmt->if_stmt->else_stmts = else_stmts;
    return stmt;
}

struct Statement *new_while(struct Value *condition, Statements *stmts) {
    struct Statement *stmt = malloc(sizeof(struct Statement));
    stmt->type = STMT_WHILE;
    stmt->while_stmt = malloc(sizeof(struct While));
    stmt->while_stmt->condition = condition;
    stmt->while_stmt->stmts = stmts;
    return stmt;
}


struct Value *new_string(char *string) {
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_STRING;
    val->string = string;
    return val;
}

struct Value *new_symbol(char *symbol) {
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_SYMBOL;
    val->symbol = symbol;
    return val;
}

struct Value *new_integer(long integer) {
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_INT;
    val->integer = integer;
    return val;
}

struct Value *new_floating(double floating) {
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_FLOAT;
    val->floating = floating;
    return val;
}

struct Value *new_boolean(int boolean) {
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_BOOL;
    val->boolean = boolean;
    return val;
}

struct Value *new_funcall(struct FunCall *funcall) {
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_FUNCALL;
    val->funcall = funcall;
    return val;
}

struct Value *new_expr(struct Expr *expr) {
    struct Value *val = malloc(sizeof(struct Value));
    val->type = VTYPE_EXPR;
    val->expr = expr;
    return val;
}

void print_value(struct Value *val);

void print_expr(struct Expr *expr) {
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

void print_value(struct Value *val) {
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

void print_statement(struct Statement *stmt) {
    switch (stmt->type) {
    case STMT_DIM:
        break;
    case STMT_SET:
        printf("%s = ", stmt->set->symbol);
        print_value(stmt->set->val);
        break;
    case STMT_IF:
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
}

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

struct List *program;
%}

%token ST_IF ST_THEN ST_ELSEIF ST_ELSE
%token ST_END ST_FOR ST_EACH ST_TO ST_IN ST_NEXT
%token ST_WHILE ST_FUNCTION ST_EXIT ST_DIM
%token ST_AS ST_STRING ST_INT ST_FLOAT ST_BOOL
%token ST_AND ST_OR ST_NOT ST_NULL ST_PLUS 
%token ST_MINUS ST_STAR ST_SLASH ST_EQ ST_EQEQ ST_NOT_EQ
%token ST_LESS_EQ ST_GREATER_EQ ST_LESS ST_GREATER
%token ST_OPEN_PAREN ST_CLOSE_PAREN ST_COMMA
%token ST_AMP ST_UNDERSCORE ST_SEMICOLON ST_NEWLINE

//%left ST_MINUS ST_PLUS
%left ST_OR
%left ST_AND
%left ST_EQEQ ST_NOT_EQ
%left ST_GREATER_EQ ST_GREATER ST_LESS_EQ ST_LESS
%left ST_PLUS ST_MINUS
%left ST_STAR ST_SLASH
%left UNARY_PLUS UNARY_MINUS

%token T_STRING T_SYMBOL T_INT T_FLOAT T_OTHER

%union {
    char *string;
    char *symbol;
    long integer;
    struct Expr *expr;
    struct Value *val;
    double floating;
    struct List *stmts;
    struct Statement *stmt;
}

%type <integer> T_INT
%type <symbol> T_SYMBOL
%type <stmts> program
%type <stmts> statements
%type <stmt> statement
%type <val> expr
%type <val> subexpr

// %type <name> STRING

// %type <integer> expr
// %type <integer> stmt
// %type <floating> NUM


%error-verbose

%%

program: statements { printf("this happens?\n"); program = $1; }
;

statements: statement ST_NEWLINE { print_statement($1); printf("\n"); $$ = new_list($1); }
    | statement ST_NEWLINE statements { print_statement($1); printf("\n"); $$ = push($3, $1); }
;

statement: T_SYMBOL ST_EQ expr { $$ = new_set($1, $3); }
         | ST_IF expr ST_THEN ST_NEWLINE statements ST_END ST_IF
            { $$ = new_if($2, $5, NULL); }
         | ST_IF expr ST_THEN ST_NEWLINE statements 
            ST_ELSE ST_NEWLINE statements ST_END ST_IF
            { $$ = new_if($2, $5, $8); }
         | ST_WHILE expr ST_NEWLINE statements ST_END ST_WHILE { $$ = new_while($2, $4); }
;

expr: T_INT { $$ = new_integer($1); }
    | ST_OPEN_PAREN subexpr ST_CLOSE_PAREN { $$ = $2; }
    | subexpr
;

subexpr: ST_PLUS expr %prec UNARY_PLUS { $$ = new_expr(unary_plus($2)); }
    | ST_MINUS expr %prec UNARY_MINUS { $$ = new_expr(unary_minus($2)); }
    | expr ST_PLUS expr { $$ = new_expr(bin_plus($1, $3)); }
    | expr ST_MINUS expr { $$ = new_expr(bin_minus($1, $3)); }
    | expr ST_STAR expr { $$ = new_expr(bin_star($1, $3)); }
    | expr ST_SLASH expr { $$ = new_expr(bin_slash($1, $3)); }
    | expr ST_OR expr { $$ = new_expr(bin_or($1, $3)); }
    | expr ST_AND expr { $$ = new_expr(bin_and($1, $3)); }
    | expr ST_EQEQ expr { $$ = new_expr(bin_eqeq($1, $3)); }
    | expr ST_NOT_EQ expr { $$ = new_expr(bin_not_eq($1, $3)); }
    | expr ST_GREATER_EQ expr { $$ = new_expr(bin_greater_eq($1, $3)); }
    | expr ST_LESS_EQ expr { $$ = new_expr(bin_less_eq($1, $3)); }
    | expr ST_GREATER expr { $$ = new_expr(bin_greater($1, $3)); }
    | expr ST_LESS expr { $$ = new_expr(bin_less($1, $3)); }
;


%%

int main()
{
    int val = yyparse();
    printf("yyparse: %d\n", val);
    return 0;
}

