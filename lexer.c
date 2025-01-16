#include "common.h"
#include "readfile.h"
#include "allocator.h"
#include "linkedlist.h"
#include "lexer.h"

struct TokenMapping {
    char *string;
    enum TokenType type;
};

struct TokenMapping keywords[] = {
    { "if",          ST_IF },
    { "else",        ST_ELSE },
    { "for",         ST_FOR },
    { "in",          ST_IN },
    { "while",       ST_WHILE },
    { "function",    ST_FUNCTION },
    { "get",         ST_GET },
    { "set",         ST_SET },
    { "constructor", ST_CONSTRUCTOR },
    { "let",         ST_LET },
    { "new",         ST_NEW },
    { "string",      ST_STRING },
    { "int",         ST_INT },
    { "float",       ST_FLOAT },
    { "bool",        ST_BOOL },
    { "null",        ST_NULL },
    { "class",       ST_CLASS },
    { "return",      ST_RETURN },
    { "break",       ST_BREAK },
    { "continue",    ST_CONTINUE },
    { "true",        ST_TRUE },
    { "false",       ST_FALSE }
};

struct TokenMapping symbols2[] = {
    { "&&",       ST_AND },
    { "||",       ST_OR },
    { "==",       ST_EQEQ },
    { "!=",       ST_NOT_EQ },
    { "<=",       ST_LESS_EQ },
    { ">=",       ST_GREATER_EQ },
    { "++",       ST_INC },
    { "--",       ST_DEC }
};

struct TokenMapping symbols1[] = {
    { "!",        ST_NOT },
    { "+",        ST_PLUS },
    { "-",        ST_MINUS },
    { "*",        ST_STAR },
    { "/",        ST_SLASH },
    { "%",        ST_PERCENT },
    { "=",        ST_EQ },
    { "<",        ST_LESS },
    { ">",        ST_GREATER },
    { "(",        ST_OPEN_PAREN },
    { ")",        ST_CLOSE_PAREN },
    { "{",        ST_OPEN_BRACE },
    { "}",        ST_CLOSE_BRACE },
    { "[",        ST_OPEN_BRACKET },
    { "]",        ST_CLOSE_BRACKET },
    { ",",        ST_COMMA },
    { "&",        ST_AMP },
    { ";",        ST_SEMICOLON },
    { ":",        ST_COLON },
    { ".",        ST_DOT }
};

static struct Token *new_token(struct Globals *globals, int start, int end, enum TokenType type)
{
    struct Token *token = allocator_malloc(globals->allocator, sizeof(*token));
    token->start = start;
    token->end = end;
    token->line_number = globals->lexer->error.line_number;
    token->column_number = globals->lexer->error.column_number;
    token->type = type;
    return token;
}

static struct Token *new_symbol_token(struct Globals *globals, int start, int end, Symbol symbol)
{
    struct Token *token = new_token(globals, start, end, T_SYMBOL);
    token->symbol = symbol;
    return token;
}

static struct Token *new_integer_token(struct Globals *globals, int start, int end,
        uint64_t integer)
{
    struct Token *token = new_token(globals, start, end, T_INT);
    token->integer = integer;
    return token;
}

static struct Token *new_floating_token(struct Globals *globals, int start, int end,
        double floating)
{
    struct Token *token = new_token(globals, start, end, T_FLOAT);
    token->floating = floating;
    return token;
}

static struct Token *new_string_token(struct Globals *globals, int start, int end, char *str)
{
    struct Token *token = new_token(globals, start, end, T_STRING);
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

static inline char peek(struct Globals *globals)
{
    return globals->file->input[globals->lexer->offset];
}

static inline bool has_next(struct Globals *globals)
{
    return globals->lexer->offset <= globals->file->length && peek(globals) != '\0';
}

static inline char next(struct Globals *globals)
{
    globals->lexer->error.column_number++;
    if (globals->file->input[globals->lexer->offset] == '\n') {
        globals->lexer->error.column_number = 1;
        globals->lexer->error.line_number++;
    }
    return globals->file->input[globals->lexer->offset++];
}

// Checks if lexer matches given string, consuming the characters it reads on success
// Returns the number of characters consumed on success, otherwise return 0. 
// 'require_terminal' should be set to true if it expects to be terminated by a terminal
// and set to false if not. 
// For example if we want to parse 'for' 'forever' should not be a match so we should require it
// to end in a terminal. If we parse '>=', it's okay if the next character is not a terminal  
static int match(struct Globals *globals, char *str, bool require_terminal)
{
    int init_offset = globals->lexer->offset;
    int i = globals->lexer->offset;
    while (true) {
        char c = globals->file->input[i];
        char strc = str[i - init_offset];
        if (strc == '\0' && (!require_terminal || is_terminal(c))) {
            break;
        }
        if (strc != c) {
            return 0;
        }
        i++;
    }
    globals->lexer->offset = i;
    globals->lexer->error.column_number += i - init_offset;
    return i - init_offset;
}

static void tokenize_comment(struct Globals *globals)
{
    char c = '\0';
    int init_offset = globals->lexer->offset;
    while (peek(globals) != '\n' && peek(globals) != '\0') {
        c = next(globals);
    }
    if (globals->lexer->include_comments) {
        if (c == '\r') {
            linkedlist_append(globals->lexer->tokens, new_token(globals, init_offset, globals->lexer->offset - 1, T_COMMENT));
        } else {
            linkedlist_append(globals->lexer->tokens, new_token(globals, init_offset, globals->lexer->offset, T_COMMENT));
        }
    }
}

static char *make_string(struct Globals *globals, char *input, int start, int end)
{
    char *str = allocator_malloc(globals->allocator, sizeof(char) * (end - start + 1));
    for (int i = start; i < end; i++) {
        str[i - start] = input[i];
    }
    str[end - start] = '\0';
    return str;
}

// TODO: handle escape sequences
static void tokenize_string(struct Globals *globals)
{
    int init_offset = globals->lexer->offset;
    while (peek(globals) != '"') {
        if (peek(globals) == '\n' || peek(globals) == '\0') {
            globals->lexer->error.message = "Unexpected end of string";
            return;
        }
        next(globals);
    }
    char *str = make_string(globals, globals->file->input, init_offset, globals->lexer->offset);
    linkedlist_append(globals->lexer->tokens, new_string_token(globals, init_offset, globals->lexer->offset, str));
    next(globals);
}

static bool match_simple_token_list(struct Globals *globals, struct TokenMapping *tm_list, size_t length,
    bool require_terminal)
{
    int init_offset = globals->lexer->offset;
    for (size_t i = 0; i < length; i++) {
        struct TokenMapping tm = tm_list[i];
        if (match(globals, tm.string, require_terminal)) {
            linkedlist_append(globals->lexer->tokens, new_token(globals, init_offset, globals->lexer->offset, tm.type));
            return true;
        }
    }
    return false;
}

#define match_simple_token_list_m(globals, tm_list, require_terminal) \
    match_simple_token_list(globals, tm_list,\
        (sizeof(tm_list) / sizeof(struct TokenMapping)), require_terminal)

static bool match_simple_token(struct Globals *globals)
{
    return match_simple_token_list_m(globals, symbols2, false) ||
           match_simple_token_list_m(globals, symbols1, false) ||
           match_simple_token_list_m(globals, keywords, true);
}

#undef match_simple_token_list_m

// Convert string to int64_t. Assumes null terminated string with valid characters
static int64_t str_to_int64(struct Globals *globals, int init_offset, int count)
{
    const int64_t INT64_MAX_HIGHPART = 922337203685477580;
    const int64_t INT64_MAX_LOWPART  = 7;
    int64_t num = 0;
    for (int i = init_offset; i < init_offset + count; i++) {
        int64_t digit = globals->file->input[i] - '0';
        // If we're about to overflow, return with an error
        if (num > INT64_MAX_HIGHPART || (num == INT64_MAX_HIGHPART && digit > INT64_MAX_LOWPART)) {
            globals->lexer->error.message = "integer literal exceeds maximum allowed size of integer";
            return 0;
        }
        num = num * 10 + digit;
    }
    return num;
}

static bool check_digit_part(struct Globals *globals)
{
    while (isdigit(peek(globals))) {
        next(globals);
    }
    if (!is_terminal(peek(globals))) {
        globals->lexer->error.message = "unexpected character in numeric literal";
        return false;
    }
    return true;
}

static void tokenize_number(struct Globals *globals)
{
    int init_offset = globals->lexer->offset;
    if (!check_digit_part(globals)) {
        return;
    }
    struct Token *token = NULL;
    if (peek(globals) == '.') {
        // float
        // Currently only parses floats of the form nnn.nnn
        next(globals);
        if (!check_digit_part(globals)) {
            return;
        }
        char *end;
        double num = strtod(globals->file->input + init_offset, &end);
        assert(end == globals->file->input + globals->lexer->offset);
        errno = 0; // reset in event that it overflows (we don't care)
        token = new_floating_token(globals, init_offset, globals->lexer->offset, num);
    } else {
        // int
        int count = globals->lexer->offset - init_offset;
        int64_t num = str_to_int64(globals, init_offset, count);
        token = new_integer_token(globals, init_offset, globals->lexer->offset, num);
    }
    if (!globals->lexer->error.message) {
        linkedlist_append(globals->lexer->tokens, token);
    }
}

static bool is_symbol_token(char c)
{
    return (isalnum(c) || c == '_');
}

static Symbol add_symbol(struct Globals *globals, char *yytext, int yyleng)
{
    if (globals->symbol_table == NULL) init_symbol_table(globals);
    /* Adds symbols to symbol table */
    struct LinkedListNode *symbol_table = globals->symbol_table->head;
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
    symbol = allocator_malloc(globals->allocator, (yyleng + 1) * sizeof(char));
    strcpy(symbol, yytext);
    // symbol_table->next = new_list(symbol);
    linkedlist_append(globals->symbol_table, symbol);
addsymbol:
    if (strcmp(yytext, "main") == 0) {
        globals->entrypoint = cnt;
    }
    return cnt;
}

static void tokenize_symbol(struct Globals *globals)
{
    int init_offset = globals->lexer->offset;
    while (is_symbol_token(peek(globals))) {
        next(globals);
    }
    if (!is_terminal(peek(globals))) {
        globals->lexer->error.message = "unexpected character in integer literal";
        return;
    }
    int count = globals->lexer->offset - init_offset;
    char *text = allocator_malloc(globals->allocator, count + 1);
    memcpy(text, globals->file->input + init_offset, count);
    text[count] = '\0';
    Symbol symbol = add_symbol(globals, text, count);
    linkedlist_append(globals->lexer->tokens, new_symbol_token(globals, init_offset, globals->lexer->offset, symbol));
}

void print_token(struct Globals *globals, struct Token *token)
{
    printf("Type: %d, Start: %d, End: %d, Content: '", token->type, token->start, token->end);
    for (int i = token->start; i < token->end; i++) {
        if (globals->file->input[i] == '\0') printf("**NULL**");
        if (globals->file->input[i] == '\n') printf("**NEWLINE**");
        printf("%c", globals->file->input[i]);
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
            printf(", Value: '%s'", lookup_symbol(globals, token->symbol));
            break;
        default:
            break;
    }
    printf("\n");
}

void print_lexer(struct Globals *globals, struct Lexer *lexer)
{
    printf("lexer {\n");
    printf("  [\n");
    for (struct LinkedListNode *node = lexer->tokens->head; node != NULL; node = node->next) {
        printf("    ");
        print_token(globals, node->value);
    }
    printf("  ]\n");
    printf("}\n");
}

void lexer_init(struct Globals *globals, struct Lexer *lexer, bool include_comments)
{
    lexer->error.message = NULL;
    lexer->error.line_number = 1;
    lexer->error.column_number = 1;
    // lexer->input = input;
    // lexer->input_length = input_length;
    lexer->include_comments = include_comments;
    lexer->offset = 0;
    lexer->tokens = linkedlist_new(globals->allocator);
}

bool tokenize(struct Globals *globals)
{
    if (!has_next(globals)) {
        globals->lexer->error.message = "File is empty";
        return false;
    }
    while (has_next(globals)) {
        if (is_space(peek(globals))) {
            // ignore whitespace
            next(globals);
        } else if (match(globals, "//", false)) {
            tokenize_comment(globals);
        } else if (isdigit(peek(globals))) {
            tokenize_number(globals);
        } else if (peek(globals) ==  '"') {
            next(globals);
            tokenize_string(globals);
        } else if (match_simple_token(globals)) {
            // Do nothing, match_simple_token will consume the token on success
        } else if (isalpha(peek(globals)) || peek(globals) == '_') {
            tokenize_symbol(globals);
        } else {
            globals->lexer->error.message = "illegal characters in token";
        }
        if (globals->lexer->error.message) {
            return false;
        }
    }
    return true;
}
