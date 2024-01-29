%{
#include "ast.h"
#define YYDEBUG 1

int yylex();
int yyerror(const char *s);

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
%token ST_COMMENT

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
    struct List *definitions;
    struct List *args;
    struct Definition *definition;
    struct Statement *stmt;
    enum Type type;
}

%type <integer> T_INT
%type <symbol> T_SYMBOL
%type <string> T_STRING
%type <stmts> program
%type <stmts> statements
%type <stmt> statement
%type <type> type
%type <definitions> definitions
%type <definition> definition
%type <val> expr
%type <args> args
%type <val> subexpr

// %type <name> STRING

// %type <integer> expr
// %type <integer> stmt
// %type <floating> NUM

%define parse.error detailed
%require "3.8.2"

%%


program: statements { ast.ast = reset_list_head($1); }
       | end_of_line program { $$ = $2; }
;

statements: statement end_of_line { $$ = new_list($1); }
          | statement end_of_line statements { $$ = append($3, $1); }
;

statement: T_SYMBOL ST_EQ expr { $$ = new_set($1, $3); }
         | ST_IF expr ST_THEN end_of_line statements ST_END ST_IF
            { $$ = new_if($2, $5, NULL); }
         | ST_IF expr ST_THEN end_of_line statements 
            ST_ELSE end_of_line statements ST_END ST_IF
            { $$ = new_if($2, $5, $8); }
         | ST_WHILE expr end_of_line statements ST_END ST_WHILE { $$ = new_while($2, $4); }
         | ST_DIM definitions { $$ = new_dim($2); }
         | T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_sfuncall($1, $3); }
;

definitions: definition { $$ = new_list($1); }
           | definition ST_COMMA definitions { $$ = append($3, $1); }
;

definition: T_SYMBOL ST_AS type { $$ = new_define($1, $3); }
;

type: ST_INT { $$ = TYPE_INT; }
    | ST_FLOAT { $$ = TYPE_FLOAT; }
    | ST_BOOL { $$ = TYPE_BOOL; }
    | ST_STRING { $$ = TYPE_STRING; };

expr: T_INT { $$ = new_integer($1); }
    | T_SYMBOL { $$ = new_symbol($1); }
    | T_STRING { $$ = new_string($1); }
    | ST_OPEN_PAREN subexpr ST_CLOSE_PAREN { $$ = $2; }
    | T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_vfuncall($1, $3); }
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

end_of_line: newline_or_comment
    | end_of_line newline_or_comment
    ;

newline_or_comment: ST_COMMENT | ST_NEWLINE;

%%

