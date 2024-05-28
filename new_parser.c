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
    ST_FALSE
};

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

struct Token {
    int start, end;
    enum TokenType type;
    union {
        Symbol symbol;
        uint64_t integer;
        double floating;
    };
};

typedef struct List Tokens;

struct CompilerError {
    char *message;
    int line_number;
    int column_number;
};

struct Lexer {
    struct CompilerError *error;
    int input_length; // Excludes null terminator
    char *input;
    int offset;
    Tokens *tokens;
};

static struct Token *new_token(int start, int end, enum TokenType type)
{
    struct Token *token = malloc(sizeof(*token));
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

static struct Token *new_floating_token(int start, int end, double floating)
{
    struct Token *token = new_token(start, end, T_FLOAT);
    token->floating = floating;
    return token;
}

static void print_lexer(struct Lexer *lexer)
{
    printf("lexer {\n");
    printf("  [\n");
    for (Tokens *tokens = seek_end(lexer->tokens); tokens != NULL; tokens = tokens->prev) {
        struct Token *token = tokens->value;
        printf("    Type: %d, Start: %d, End: %d, Content: '", token->type, token->start, token->end);
        for (int i = token->start; i < token->end; i++) {
            if (lexer->input[i] == '\0') printf("**NULL**");
            if (lexer->input[i] == '\n') printf("**NEWLINE**");
            printf("%c", lexer->input[i]);
        }
        printf("'");
        if (token->type == T_INT) {
            printf(", Value: '%" PRIi64 "'", token->integer);
        }
        printf("\n");
    }
    printf("  ]\n");
    printf("}\n");
}

static void print_error(struct CompilerError *error)
{
    printf("Error at line %d column %d: %s\n", error->line_number, error->column_number, error->message);
}

void add_token(struct Lexer *lexer, struct Token *token)
{
    if (lexer->tokens) {
        lexer->tokens = append(lexer->tokens, token);
    } else {
        lexer->tokens = new_list(token);
    }
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

static inline bool has_next(struct Lexer *lexer)
{
    return lexer->offset <= lexer->input_length;
}

static inline char next(struct Lexer *lexer)
{
    lexer->error->column_number++;
    if (lexer->input[lexer->offset] == '\n') {
        lexer->error->column_number = 1;
        lexer->error->line_number++;
    }
    return lexer->input[lexer->offset++];
}

static inline char peek(struct Lexer *lexer)
{
    return lexer->input[lexer->offset];
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
    lexer->error->column_number += i - init_offset;
    return i - init_offset;
}

int tokenize_symbols(struct Lexer *lexer, int offset)
{
    char c = lexer->input[offset];
    char next = lexer->input_length < offset + 1 ? lexer->input[offset+1] : '\0';
    if (c == '&' && next == '&') {
    }
}

void tokenize_comment(struct Lexer *lexer)
{
    int init_offset = lexer->offset;
    while (peek(lexer) != '\n' && peek(lexer) != '\0') {
        next(lexer);
    }
    if (peek(lexer) == '\n') {
        add_token(lexer, new_token(init_offset, lexer->offset - 1, T_COMMENT));
    } else {
        add_token(lexer, new_token(init_offset, lexer->offset, T_COMMENT));
    }
}

void tokenize_string(struct Lexer *lexer)
{
    int init_offset = lexer->offset;
    while (peek(lexer) != '"') {
        if (peek(lexer) == '\n' || peek(lexer) == '\0') {
            lexer->error->message = "Unexpected end of string";
            return;
        }
        next(lexer);
    }
    add_token(lexer, new_token(init_offset, lexer->offset, T_STRING));
    next(lexer);
}


bool match_simple_token_list(struct Lexer *lexer, struct TokenMapping *tm_list, size_t length,
    bool require_terminal)
{
    int init_offset = lexer->offset;
    for (size_t i = 0; i < length; i++) {
        struct TokenMapping tm = tm_list[i];
        if (match(lexer, tm.string, require_terminal)) {
            add_token(lexer, new_token(init_offset, lexer->offset, tm.type));
            return true;
        }
    }
    return false;
}

#define match_simple_token_list_m(lexer, tm_list, require_terminal) \
    match_simple_token_list(lexer, tm_list,\
        (sizeof(tm_list) / sizeof(struct TokenMapping)), require_terminal)

bool match_simple_token(struct Lexer *lexer)
{
    return match_simple_token_list_m(lexer, symbols2, false) ||
           match_simple_token_list_m(lexer, symbols1, false) ||
           match_simple_token_list_m(lexer, keywords, true);
}

#undef match_simple_token_list_m

#define INT64_MAX_HIGHPART 922337203685477580
#define INT64_MAX_LOWPART 7

// Convert string to int64_t. Assumes null terminated string with valid characters
int64_t str_to_int64(struct Lexer *lexer, int init_offset, int count)
{
    int64_t num = 0;
    for (int i = init_offset; i < init_offset + count; i++) {
        int64_t digit = lexer->input[i] - '0';
        // If we're about to overflow, return with an error
        if (num > INT64_MAX_HIGHPART || (num == INT64_MAX_HIGHPART && digit > INT64_MAX_LOWPART)){
            lexer->error->message = "integer literal exceeds maximum allowed size of integer";
            return 0;
        }
        num = num * 10 + digit;
    }
    return num;
}

// TODO: make this work for double
void tokenize_number(struct Lexer *lexer)
{
    int init_offset = lexer->offset;
    while (isdigit(peek(lexer))) {
        next(lexer);
    }
    if (!is_terminal(peek(lexer))) {
        lexer->error->message = "unexpected character in integer literal";
        return;
    }
    int count = lexer->offset - init_offset;
    int64_t num = str_to_int64(lexer, init_offset, count);
    if (!lexer->error->message) {
        add_token(lexer, new_integer_token(init_offset, lexer->offset, num));
    }
}

bool is_symbol_token(char c)
{
    return (isalnum(c) || c == '_');
}

Symbol add_symbol(char *yytext, int yyleng)
{
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
    return cnt;
}

void tokenize_symbol(struct Lexer *lexer)
{
    int init_offset = lexer->offset;
    while (is_symbol_token(peek(lexer))) {
        next(lexer);
    }
    if (!is_terminal(peek(lexer))) {
        lexer->error->message = "unexpected character in integer literal";
        return;
    }
    int count = lexer->offset - init_offset;
    char *text = malloc(count + 1);
    memcpy(text, lexer->input + init_offset, count);
    text[count] = '\0';
    Symbol symbol = add_symbol(text, count);
    add_token(lexer, new_symbol_token(init_offset, lexer->offset, symbol));
    free(text);
}

void tokenize(struct Lexer *lexer)
{
    if (!has_next(lexer)) {
        lexer->error->message = "File is empty";
        return;
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
            lexer->error->message = "illegal characters in token";
        }
        if (lexer->error->message) {
            return;
        }
    }
}

// Populates buffer with contents of file
// Returns 0 on failure, returns length of buffer (excluding null terminator) on success
int readfile(char **buff, char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: could not open file '%s'\n", filename);
        return 0;
    }
    char *generic_error = "Error: encountered error while reading file\n";
    if (fseek(fp, 0, SEEK_END) != 0) {
        printf(generic_error);
        return 0;
    }
    long length = ftell(fp);
    if (length == -1) {
        printf(generic_error);
        return 0;
    }
    rewind(fp);

    *buff = malloc(length + 1);
    if ((long) fread(*buff, 1, length, fp) != length) {
        printf(generic_error);
        return 0;
    }
    fclose(fp);

    (*buff)[length] = '\0';
    return length - 1;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage: ./del hello_world.del\n");
        return 1;
    }
    printf("........ READING FILE : %s ........\n", argv[1]);
    char *input = NULL;
    int input_length = readfile(&input, argv[1]);
    if (input_length == 0) {
        return 1;
    }
    printf("%s\n", input);
    printf("........ TOKENIZING INPUT ........\n");
    struct CompilerError error = { NULL, 1, 1 };
    struct Lexer lexer = {
        &error,
        input_length,
        input,
        0,
        NULL
    };
    tokenize(&lexer);
    if (lexer.error->message) {
        print_error(lexer.error);
        return 1;
    }
    print_lexer(&lexer);

    return 0;
}
