%{
#include "ast.h"
#define YYDEBUG 1

int yylex();
int yyerror(const char *s);

%}

%token ST_IF ST_ELSE
%token ST_FOR ST_IN
%token ST_WHILE ST_FUNCTION ST_LET ST_NEW
%token ST_STRING ST_INT ST_FLOAT ST_BOOL ST_NULL
%token ST_AND ST_OR ST_NOT
%token ST_PLUS ST_MINUS ST_STAR ST_SLASH
%token ST_EQ ST_EQEQ ST_NOT_EQ
%token ST_LESS_EQ ST_GREATER_EQ ST_LESS ST_GREATER
%token ST_OPEN_PAREN ST_CLOSE_PAREN ST_COMMA
%token ST_AMP ST_UNDERSCORE ST_SEMICOLON ST_NEWLINE
%token ST_COMMENT
%token ST_CLASS ST_RETURN
%token ST_COLON ST_OPEN_BRACE ST_CLOSE_BRACE ST_OPEN_BRACKET ST_CLOSE_BRACKET
%token ST_DOT
%token ST_TRUE ST_FALSE

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
    Symbol symbol;
    int64_t integer;
    struct Expr *expr;
    struct Value *val;
    double floating;
    struct List *stmts;
    struct List *tlds;
    struct List *methods;
    struct List *definitions;
    struct List *symbols;
    struct List *args;
    struct FunDef *method;
    struct Definition *definition;
    struct Statement *stmt;
    struct TopLevelDecl *tld;
    Type type;
    struct List *lvalues;
    struct LValue *lvalue;
}

%type <integer> T_INT
%type <symbol> T_SYMBOL
%type <string> T_STRING
%type <tlds> program
%type <tlds> tlds
%type <methods> methods
%type <tld> tld
%type <tld> cls
%type <tld> fundef
%type <method> method
%type <stmts> statements
%type <stmts> block
%type <stmt> statement
%type <type> type
%type <definitions> class_definitions
%type <definitions> symbols
%type <symbols> definitions
%type <definition> definition
%type <val> expr
%type <args> args
%type <val> subexpr
%type <lvalues> accessors
%type <lvalue> accessor

%define parse.error detailed
%require "3.8.2"
%%

program: tlds { ast.ast = $1; };

tlds: tld { $$ = new_list($1); }
    | tld tlds { $$ = append($2, $1); }
;

tld: fundef { $$ = $1; } | cls { $$ = $1; };

fundef: ST_FUNCTION T_SYMBOL ST_OPEN_PAREN definitions ST_CLOSE_PAREN ST_OPEN_BRACE
        statements ST_CLOSE_BRACE { $$ = new_tld_fundef($2, $4, $7); }
;

cls: ST_CLASS T_SYMBOL ST_OPEN_BRACE class_definitions methods ST_CLOSE_BRACE
       { $$ = new_class($2, $4, $5); }
   | ST_CLASS T_SYMBOL ST_OPEN_BRACE class_definitions ST_CLOSE_BRACE
       { $$ = new_class($2, $4, NULL); }
   | ST_CLASS T_SYMBOL ST_OPEN_BRACE methods ST_CLOSE_BRACE
       { $$ = new_class($2, NULL, $4); }
;

class_definitions: definition ST_SEMICOLON { $$ = new_list($1); }
                 | definition ST_SEMICOLON class_definitions { $$ = append($3, $1); }
;

methods: method { $$ = new_list($1); }
       | method methods { $$ = append($2, $1); }
;

method: ST_FUNCTION T_SYMBOL ST_OPEN_PAREN definitions ST_CLOSE_PAREN ST_OPEN_BRACE
        statements ST_CLOSE_BRACE { $$ = new_fundef($2, $4, $7); }
;

statements: statement { $$ = new_list($1); }
          | statement statements { $$ = append($2, $1); }
;

block: ST_OPEN_BRACE statements ST_CLOSE_BRACE { $$ = $2; };

statement: T_SYMBOL ST_EQ expr ST_SEMICOLON { $$ = new_set($1, $3, NULL, 0); }
         | T_SYMBOL accessors expr ST_SEMICOLON { $$ = new_set($1, $3, $2, 0); }
         | ST_LET T_SYMBOL ST_EQ expr ST_SEMICOLON { $$ = new_set($2, $4, NULL, 1); }
         | ST_LET T_SYMBOL accessors expr ST_SEMICOLON { $$ = new_set($2, $4, $3, 1); }
         | ST_LET symbols ST_SEMICOLON { $$ = new_let($2); }
         | T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN ST_SEMICOLON { $$ = new_sfuncall($1, $3); }
         | ST_RETURN expr ST_SEMICOLON { $$ = new_return($2); }
         | ST_IF expr block { $$ = new_if($2, $3, NULL); }
         | ST_IF expr block ST_ELSE block { $$ = new_if($2, $3, $5); }
         | ST_WHILE expr block { $$ = new_while($2, $3); }
;

accessors: accessor accessors { $$ = append($2, $1); }
         | accessor ST_EQ { $$ = new_list($1); }

accessor: ST_DOT T_SYMBOL { $$ = new_property($2); }
        | ST_OPEN_BRACKET T_INT ST_CLOSE_BRACKET { $$ = new_index(new_integer($2)); }

symbols: T_SYMBOL { $$ = new_list(new_define($1, TYPE_UNDEFINED)); }
       | T_SYMBOL ST_COMMA symbols { $$ = append($3, new_define($1, TYPE_UNDEFINED)); }
;

definitions: definition { $$ = new_list($1); }
           | definition ST_COMMA definitions { $$ = append($3, $1); }
;

definition: T_SYMBOL ST_COLON type { $$ = new_define($1, $3); };

type: ST_INT { $$ = TYPE_INT; }
    | ST_FLOAT { $$ = TYPE_FLOAT; }
    | ST_BOOL { $$ = TYPE_BOOL; }
    | ST_STRING { $$ = TYPE_STRING; }
    | T_SYMBOL { $$ = $1; }
;


expr: T_INT { $$ = new_integer($1); }
    | T_SYMBOL { $$ = new_symbol($1); }
    | T_STRING { $$ = new_string($1); }
    | ST_TRUE { $$ = new_boolean(1); }
    | ST_FALSE { $$ = new_boolean(0); }
    | ST_OPEN_PAREN subexpr ST_CLOSE_PAREN { $$ = $2; }
    | T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_vfuncall($1, $3); }
    | ST_NEW T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_vfuncall($2, $4); }
    | subexpr
;

args: expr { $$ = new_list($1); }
    | expr ST_COMMA args { $$ = append($3, $1); }
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

