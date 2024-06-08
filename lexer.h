#ifndef LEXER_H
#define LEXER_H
#include "common.h"

enum TokenType {
    T_STRING,
    T_SYMBOL,
    T_INT,
    T_FLOAT,
    T_COMMENT,
    ST_IF,
    ST_ELSE,
    ST_FOR,
    ST_IN,
    ST_WHILE,
    ST_FUNCTION,
    ST_LET,
    ST_NEW,
    ST_STRING,
    ST_INT,
    ST_FLOAT,
    ST_BOOL,
    ST_AND,
    ST_OR,
    ST_NOT,
    ST_NULL,
    ST_CLASS,
    ST_RETURN,
    ST_PLUS,
    ST_MINUS,
    ST_STAR,
    ST_SLASH,
    ST_EQ,
    ST_EQEQ,
    ST_NOT_EQ,
    ST_LESS_EQ,
    ST_GREATER_EQ,
    ST_LESS,
    ST_GREATER,
    ST_OPEN_PAREN,
    ST_CLOSE_PAREN,
    ST_OPEN_BRACE,
    ST_CLOSE_BRACE,
    ST_OPEN_BRACKET,
    ST_CLOSE_BRACKET,
    ST_COMMA,
    ST_AMP,
    ST_SEMICOLON,
    ST_COLON,
    ST_DOT,
    ST_TRUE,
    ST_FALSE,
    // Forces use of signed int, so we can do typesafe comparisons with MatchType in the parser
    UNUSED_FORCE_NEGATIVE = -1
};

struct Token {
    int start, end;
    enum TokenType type;
    union {
        Symbol symbol;
        uint64_t integer;
        double floating;
        char *string;
    };
};

typedef struct LinkedList Tokens;

struct CompilerError {
    char *message;
    int line_number;
    int column_number;
};

struct Lexer {
    struct CompilerError error;
    int input_length; // Excludes null terminator
    char *input;
    int offset;
    Tokens *tokens;
};

void lexer_init(struct Lexer *lexer, char *input, int input_length);
bool tokenize(struct Lexer *lexer);
void print_error(struct CompilerError *error); 
void print_token(struct Lexer *lexer, struct Token *token);
void print_lexer(struct Lexer *lexer);

#endif
