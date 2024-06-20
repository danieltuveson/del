#include "common.h"
#include "allocator.h"
#include "linkedlist.h"
#include "lexer.h"

struct TokenMapping {
    char *string;
    enum TokenType type;
};

struct TokenMapping keywords[] = {
    {"if",       ST_IF },
    {"else",     ST_ELSE },
    {"for",      ST_FOR },
    {"in",       ST_IN },
    {"while",    ST_WHILE },
    {"function", ST_FUNCTION },
    {"let",      ST_LET },
    {"new",      ST_NEW },
    {"string",   ST_STRING },
    {"int",      ST_INT },
    {"float",    ST_FLOAT },
    {"boolean",  ST_BOOL },
    {"null",     ST_NULL },
    {"class",    ST_CLASS },
    {"return",   ST_RETURN },
    {"true",     ST_TRUE },
    {"false",    ST_FALSE }
};

struct TokenMapping symbols2[] = {
    {"&&",       ST_AND },
    {"||",       ST_OR },
    {"==",       ST_EQEQ },
    {"!=",       ST_NOT_EQ },
    {"<=",       ST_LESS_EQ },
    {">=",       ST_GREATER_EQ }
};

struct TokenMapping symbols1[] = {
    {"!",        ST_NOT },
    {"+",        ST_PLUS },
    {"-",        ST_MINUS },
    {"*",        ST_STAR },
    {"/",        ST_SLASH },
    {"%",        ST_PERCENT },
    {"=",        ST_EQ },
    {"<",        ST_LESS },
    {">",        ST_GREATER },
    {"(",        ST_OPEN_PAREN },
    {")",        ST_CLOSE_PAREN },
    {"{",        ST_OPEN_BRACE },
    {"}",        ST_CLOSE_BRACE },
    {"[",        ST_OPEN_BRACKET },
    {"]",        ST_CLOSE_BRACKET },
    {",",        ST_COMMA },
    {"&",        ST_AMP },
    {";",        ST_SEMICOLON },
    {":",        ST_COLON },
    {".",        ST_DOT }
};

static struct Token *new_token(int start, int end, enum TokenType type)
{
    struct Token *token = allocator_malloc(sizeof(*token));
    token->start = start;
    token->end = end;
    token->type = type;
    return token;
}

static struct Token *new_symbol_token(int start, int end, Symbol symbol)
{
    struct Token *token = new_token(start, end, T_SYMBOL);
    token->symbol = symbol;
    return token;
}

static struct Token *new_integer_token(int start, int end, uint64_t integer)
{
    struct Token *token = new_token(start, end, T_INT);
    token->integer = integer;
    return token;
}

// static struct Token *new_floating_token(int start, int end, double floating)
// {
//     struct Token *token = new_token(start, end, T_FLOAT);
//     token->floating = floating;
//     return token;
// }

static struct Token *new_string_token(int start, int end, char *str)
{
    struct Token *token = new_token(start, end, T_STRING);
    token->string = str;
    return token;
}

static inline bool is_symbol(char c)
{
    return c == '|' || c == '!' || c == '+' || c == '-' || c == '*' || c == '/' || c == '=' ||
           c == '<' || c == '>' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' ||
           c == ']' || c == ',' || c == '&' || c == ';' || c == ':' || c == '.';
}

static inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline bool is_terminal(char c)
{
    return is_symbol(c) || is_space(c) || c == '\0';
}

static inline char peek(struct Lexer *lexer)
{
    return lexer->input[lexer->offset];
}

static inline bool has_next(struct Lexer *lexer)
{
    return lexer->offset <= lexer->input_length && peek(lexer) != '\0';
}

static inline char next(struct Lexer *lexer)
{
    lexer->error.column_number++;
    if (lexer->input[lexer->offset] == '\n') {
        lexer->error.column_number = 1;
        lexer->error.line_number++;
    }
    return lexer->input[lexer->offset++];
}

// Checks if lexer matches given string, consuming the characters it reads on success
// Returns the number of characters consumed on success, otherwise return 0. 
// 'require_terminal' should be set to true if it expects to be terminated by a terminal
// and set to false if not. 
// For example if we want to parse 'for' 'forever' should not be a match so we should require it
// to end in a terminal. If we parse '>=', it's okay if the next character is not a terminal  
static int match(struct Lexer *lexer, char *str, bool require_terminal)
{
    int init_offset = lexer->offset;
    int i = lexer->offset;
    while (true) {
        char c = lexer->input[i];
        char strc = str[i - init_offset];
        if (strc == '\0' && (!require_terminal || is_terminal(c))) {
            break;
        }
        if (strc != c) {
            return 0;
        }
        i++;
    }
    lexer->offset = i;
    lexer->error.column_number += i - init_offset;
    return i - init_offset;
}

static void tokenize_comment(struct Lexer *lexer)
{
    char c = '\0';
    int init_offset = lexer->offset;
    while (peek(lexer) != '\n' && peek(lexer) != '\0') {
        c = next(lexer);
    }
    if (lexer->include_comments) {
        if (c == '\r') {
            linkedlist_append(lexer->tokens, new_token(init_offset, lexer->offset - 1, T_COMMENT));
        } else {
            linkedlist_append(lexer->tokens, new_token(init_offset, lexer->offset, T_COMMENT));
        }
    }
}

static char *make_string(char *input, int start, int end)
{
    char *str = allocator_malloc(sizeof(char) * (end - start + 1));
    for (int i = start; i < end; i++) {
        str[i - start] = input[i];
    }
    str[end - start] = '\0';
    return str;
}

// TODO: handle escape sequences
static void tokenize_string(struct Lexer *lexer)
{
    int init_offset = lexer->offset;
    while (peek(lexer) != '"') {
        if (peek(lexer) == '\n' || peek(lexer) == '\0') {
            lexer->error.message = "Unexpected end of string";
            return;
        }
        next(lexer);
    }
    char *str = make_string(lexer->input, init_offset, lexer->offset);
    linkedlist_append(lexer->tokens, new_string_token(init_offset, lexer->offset, str));
    next(lexer);
}

static bool match_simple_token_list(struct Lexer *lexer, struct TokenMapping *tm_list, size_t length,
    bool require_terminal)
{
    int init_offset = lexer->offset;
    for (size_t i = 0; i < length; i++) {
        struct TokenMapping tm = tm_list[i];
        if (match(lexer, tm.string, require_terminal)) {
            linkedlist_append(lexer->tokens, new_token(init_offset, lexer->offset, tm.type));
            return true;
        }
    }
    return false;
}

#define match_simple_token_list_m(lexer, tm_list, require_terminal) \
    match_simple_token_list(lexer, tm_list,\
        (sizeof(tm_list) / sizeof(struct TokenMapping)), require_terminal)

static bool match_simple_token(struct Lexer *lexer)
{
    return match_simple_token_list_m(lexer, symbols2, false) ||
           match_simple_token_list_m(lexer, symbols1, false) ||
           match_simple_token_list_m(lexer, keywords, true);
}

#undef match_simple_token_list_m

// Convert string to int64_t. Assumes null terminated string with valid characters
static int64_t str_to_int64(struct Lexer *lexer, int init_offset, int count)
{
    const int64_t INT64_MAX_HIGHPART = 922337203685477580;
    const int64_t INT64_MAX_LOWPART  = 7;
    int64_t num = 0;
    for (int i = init_offset; i < init_offset + count; i++) {
        int64_t digit = lexer->input[i] - '0';
        // If we're about to overflow, return with an error
        if (num > INT64_MAX_HIGHPART || (num == INT64_MAX_HIGHPART && digit > INT64_MAX_LOWPART)) {
            lexer->error.message = "integer literal exceeds maximum allowed size of integer";
            return 0;
        }
        num = num * 10 + digit;
    }
    return num;
}

// TODO: make this work for double
static void tokenize_number(struct Lexer *lexer)
{
    int init_offset = lexer->offset;
    while (isdigit(peek(lexer))) {
        next(lexer);
    }
    if (!is_terminal(peek(lexer))) {
        lexer->error.message = "unexpected character in integer literal";
        return;
    }
    int count = lexer->offset - init_offset;
    int64_t num = str_to_int64(lexer, init_offset, count);
    if (!lexer->error.message) {
        linkedlist_append(lexer->tokens, new_integer_token(init_offset, lexer->offset, num));
    }
}

static bool is_symbol_token(char c)
{
    return (isalnum(c) || c == '_');
}

// TODO: rewrite this to use globals
static Symbol add_symbol(char *yytext, int yyleng)
{
    if (ast.symbol_table == NULL) init_symbol_table();
    /* Adds symbols to symbol table */
    struct LinkedListNode *symbol_table = ast.symbol_table->head;
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
    symbol = allocator_malloc((yyleng + 1) * sizeof(char));
    strcpy(symbol, yytext);
    // symbol_table->next = new_list(symbol);
    linkedlist_append(ast.symbol_table, symbol);
addsymbol:
    if (strcmp(yytext, "main") == 0) {
        ast.entrypoint = cnt;
    }
    return cnt;
}

static void tokenize_symbol(struct Lexer *lexer)
{
    int init_offset = lexer->offset;
    while (is_symbol_token(peek(lexer))) {
        next(lexer);
    }
    if (!is_terminal(peek(lexer))) {
        lexer->error.message = "unexpected character in integer literal";
        return;
    }
    int count = lexer->offset - init_offset;
    char *text = allocator_malloc(count + 1);
    memcpy(text, lexer->input + init_offset, count);
    text[count] = '\0';
    Symbol symbol = add_symbol(text, count);
    linkedlist_append(lexer->tokens, new_symbol_token(init_offset, lexer->offset, symbol));
}

void print_token(struct Lexer *lexer, struct Token *token)
{
    printf("Type: %d, Start: %d, End: %d, Content: '", token->type, token->start, token->end);
    for (int i = token->start; i < token->end; i++) {
        if (lexer->input[i] == '\0') printf("**NULL**");
        if (lexer->input[i] == '\n') printf("**NEWLINE**");
        printf("%c", lexer->input[i]);
    }
    printf("'");
    switch (token->type) {
        case T_INT:
            printf(", Value: '%" PRIi64 "'", token->integer);
            break;
        case T_STRING:
            printf(", Value: '%s'", token->string);
            break;
        case T_SYMBOL:
            printf(", Value: '%s'", lookup_symbol(token->symbol));
            break;
        default:
            break;
    }
    printf("\n");
}

void print_lexer(struct Lexer *lexer)
{
    printf("lexer {\n");
    printf("  [\n");
    for (struct LinkedListNode *node = lexer->tokens->head; node != NULL; node = node->next) {
        printf("    ");
        print_token(lexer, node->value);
    }
    printf("  ]\n");
    printf("}\n");
}

void print_error(struct CompilerError *error)
{
    printf("Error at line %d column %d: %s\n", error->line_number, error->column_number, error->message);
}

void lexer_init(struct Lexer *lexer, char *input, int input_length, bool include_comments)
{
    lexer->error.message = NULL;
    lexer->error.line_number = 1;
    lexer->error.column_number = 1;
    lexer->input = input;
    lexer->input_length = input_length;
    lexer->include_comments = include_comments;
    lexer->offset = 0;
    lexer->tokens = linkedlist_new();
}

bool tokenize(struct Lexer *lexer)
{
    if (!has_next(lexer)) {
        lexer->error.message = "File is empty";
        return false;
    }
    while (has_next(lexer)) {
        if (is_space(peek(lexer))) {
            // ignore whitespace
            next(lexer);
        } else if (match(lexer, "//", false)) {
            tokenize_comment(lexer);
        } else if (isdigit(peek(lexer))) {
            tokenize_number(lexer);
        } else if (peek(lexer) ==  '"') {
            next(lexer);
            tokenize_string(lexer);
        } else if (match_simple_token(lexer)) {
            // Do nothing, match_simple_token will consume the token on success
        } else if (isalpha(peek(lexer)) || peek(lexer) == '_') {
            tokenize_symbol(lexer);
        } else {
            lexer->error.message = "illegal characters in token";
        }
        if (lexer->error.message) {
            return false;
        }
    }
    return true;
}