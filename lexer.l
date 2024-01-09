%{
#include "ast.h"
#include "parser.tab.h"
void bad_characters();
%}

%option yylineno

digit      [0-9]
letter     [a-zA-Z]
spaces     [ \t]
newline    \n

%%

"if"        { return ST_IF; }
"then"      { return ST_THEN; }
"elseif"    { return ST_ELSEIF; }
"else"      { return ST_ELSE; }
"end"       { return ST_END; }
"for"       { return ST_FOR; }
"each"      { return ST_EACH; }
"to"        { return ST_TO; }
"in"        { return ST_IN; }
"next"      { return ST_NEXT; }
"while"     { return ST_WHILE; }
"function"  { return ST_FUNCTION; }
"exit"      { return ST_EXIT; }
"dim"       { return ST_DIM; }
"as"        { return ST_AS; }
"string"    { return ST_STRING; }
"int"       { return ST_INT; }
"float"     { return ST_FLOAT; }
"bool"      { return ST_BOOL; }
"and"       { return ST_AND; }
"or"        { return ST_OR; }
"not"       { return ST_NOT; }
"null"      { return ST_NULL; }
"+"         { return ST_PLUS; }
"-"         { return ST_MINUS; }
"*"         { return ST_STAR; }
"/"         { return ST_SLASH; }
"="         { return ST_EQ; }
"=="        { return ST_EQEQ; }
"<>"        { return ST_NOT_EQ; }
"<="        { return ST_LESS_EQ; }
">="        { return ST_GREATER_EQ; }
"<"         { return ST_LESS; }
">"         { return ST_GREATER; }
"("         { return ST_OPEN_PAREN; }
")"         { return ST_CLOSE_PAREN; }
","         { return ST_COMMA; }
"&"         { return ST_AMP; }
"_"         { return ST_UNDERSCORE; }
";"         { return ST_SEMICOLON; }
(({spaces}*)({newline}+)({spaces}*))+  { return ST_NEWLINE; }


{spaces}     /* Ignore insignificant whitespace */

({letter}|"_")({letter}{digit})* {
    yylval.symbol = malloc((yyleng + 1) * sizeof(char));
    strcpy(yylval.symbol, yytext);
    return T_SYMBOL; }

{digit}+  {
    yylval.integer = atoi(yytext);
    return (T_INT); }

.          { bad_characters(); return (T_OTHER); }

%%

void yyerror(const char *str)
{
    fprintf(stderr,"Error at line: %d\n%s\n", yylineno, str);
}


void bad_characters(){
    printf("unexpected characters in input");
}

int yywrap(){
    return 1;
}

