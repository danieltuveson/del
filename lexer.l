%{
#include "ast.h"
#include "parser.tab.h"
void bad_characters();
%}

%option yylineno

digit      [0-9]
letter     [a-zA-Z]
spaces     [ \t\n\r]
comment    \/\/[^\n]*

%%

"if"        { return ST_IF; }
"else"      { return ST_ELSE; }
"for"       { return ST_FOR; }
"in"        { return ST_IN; }
"while"     { return ST_WHILE; }
"function"  { return ST_FUNCTION; }
"let"       { return ST_LET; }
"new"       { return ST_NEW; }
"string"    { return ST_STRING; }
"int"       { return ST_INT; }
"float"     { return ST_FLOAT; }
"boolean"   { return ST_BOOL; }
"&&"        { return ST_AND; }
"||"        { return ST_OR; }
"!"         { return ST_NOT; }
"null"      { return ST_NULL; }
"class"     { return ST_CLASS; }
"return"    { return ST_RETURN; }
"+"         { return ST_PLUS; }
"-"         { return ST_MINUS; }
"*"         { return ST_STAR; }
"/"         { return ST_SLASH; }
"="         { return ST_EQ; }
"=="        { return ST_EQEQ; }
"!="        { return ST_NOT_EQ; }
"<="        { return ST_LESS_EQ; }
">="        { return ST_GREATER_EQ; }
"<"         { return ST_LESS; }
">"         { return ST_GREATER; }
"("         { return ST_OPEN_PAREN; }
")"         { return ST_CLOSE_PAREN; }
"{"         { return ST_OPEN_BRACE; }
"}"         { return ST_CLOSE_BRACE; }
"["         { return ST_OPEN_BRACKET; }
"]"         { return ST_CLOSE_BRACKET; }
","         { return ST_COMMA; }
"&"         { return ST_AMP; }
";"         { return ST_SEMICOLON; }
":"         { return ST_COLON; }
"."         { return ST_DOT; }
{spaces}     /* Ignore insignificant whitespace */

"true"      { return ST_TRUE; }
"false"     { return ST_FALSE; }

{letter}+ {
    if (ast.symbol_table == NULL) init_symbol_table();
    /* Adds symbols to symbol table */
    struct List *symbol_table = ast.symbol_table;
    uint64_t cnt = 0;
    char *symbol;
    while (1) {
        if (strcmp(symbol_table->value, yytext) == 0) {
            goto addsymbol;
        }
        cnt++;
        if (symbol_table->next == NULL) {
            break;
        } else {
            symbol_table = symbol_table->next;
        }
    }
    symbol = malloc((yyleng + 1) * sizeof(char));
    strcpy(symbol, yytext);
    symbol_table->next = new_list(symbol);

addsymbol:
    if (strcmp(yytext, "main") == 0) {
        ast.entrypoint = cnt;
    }
    yylval.symbol = cnt;
    return T_SYMBOL;

}

\"[^\"]*\" {
    yylval.string = malloc((yyleng - 1) * sizeof(char));
    for (unsigned long i = 1; i < yyleng - 1; i++) {
        yylval.string[i - 1] = yytext[i];
    }
    yylval.string[yyleng - 2] = '\0';
    return T_STRING;
}

{digit}+  {
    yylval.integer = atoi(yytext);
    return (T_INT); }

({comment})+

.          { bad_characters(yytext); return (T_OTHER); }

%%

void yyerror(const char *str)
{
    fprintf(stderr,"Error at line: %d\n%s\n", yylineno, str);
}


void bad_characters(char *text)
{
    printf("unexpected input:'%s'", text);
}

int yywrap()
{
    return 1;
}

