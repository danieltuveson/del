#include <assert.h>
#include "printers.h"
#include "parser.h"


struct ParserAllocator {
    size_t offset;
    unsigned char *memory;
};

static struct ParserAllocator allocator;

// Not possible to parse more Exprs than there are characters.
// It's likely a big overestimate, but makes it possible to allocate everything upfront.
void *init_allocator(size_t input_string_size)
{
    allocator.offset = 0;
    allocator.memory = calloc(input_string_size, sizeof(struct Expr));
    return allocator.memory;
}

void *alloc(size_t size)
{
    void *memory = &allocator.memory[allocator.offset];
    allocator.offset += size;
    return memory;
}

enum SimpleToken {
    ST_IF = 0, ST_THEN = 1, ST_ELSEIF = 2, ST_ELSE = 3, ST_END = 4, ST_FOR = 5,
    ST_TO = 6, ST_IN = 7, ST_NEXT = 8, ST_WHILE = 9, ST_FUNCTION = 10, ST_EXIT = 11,
    ST_DIM = 12, ST_AS = 13, ST_STRING = 14, ST_INT = 15, ST_FLOAT = 16,
    ST_BOOL = 17, ST_AND = 18, ST_OR = 19, ST_NOT = 20, ST_NULL = 21, ST_PLUS = 22,
    ST_MINUS = 23, ST_STAR = 24, ST_SLASH = 25, ST_EQ = 26, ST_LESS_EQ = 26,
    ST_GREATER_EQ = 28, ST_LESS = 29, ST_GREATER = 30, ST_OPEN_PAREN = 31,
    ST_CLOSE_PAREN = 32, ST_COMMA = 33, ST_AMP = 34, ST_UNDERSCORE = 35,
    ST_NEWLINE = 36
};

enum TokenType {
    TT_SIMPLE,
    TT_COMMENT,
    TT_STRING,
    TT_NUMBER,
    TT_SYMBOL
};

struct Tokens;
struct Tokens {
    int start;
    int end;
    enum TokenType type;
    union {
        enum SimpleToken st;
        char *string; // used for string, comment, and symbol
                      // interpretation of this value is dependent on token type
        double number;
    };
    struct Tokens *next;
};

#define init_generic_token(tokens, start, end) do {\
    tokens->start = start;\
    tokens->end = end;\
    tokens->next = NULL; } while (0)

static inline struct Tokens *simple_token(enum SimpleToken st, int start, int end) {
    struct Tokens *tokens = alloc(sizeof(struct Tokens));
    init_generic_token(tokens, start, end);
    tokens->type = TT_SIMPLE;
    tokens->st = st;
    return tokens;
}
static inline struct Tokens *comment_token(char *str, int start, int end) {
    struct Tokens *tokens = alloc(sizeof(struct Tokens));
    init_generic_token(tokens, start, end);
    tokens->start = start;
    tokens->end = end;
    tokens->type = TT_COMMENT;
    tokens->string = str;
    return tokens;
}
static inline struct Tokens *string_token(char *str, int start, int end) {
    struct Tokens *tokens = alloc(sizeof(struct Tokens));
    init_generic_token(tokens, start, end);
    tokens->type = TT_STRING;
    tokens->string = str;
    return tokens;
}
static inline struct Tokens *number_token(double num, int start, int end) {
    struct Tokens *tokens = alloc(sizeof(struct Tokens));
    init_generic_token(tokens, start, end);
    tokens->type = TT_NUMBER;
    tokens->number = num;
    return tokens;
}
static inline struct Tokens *symbol_token(char *str, int start, int end) {
    struct Tokens *tokens = alloc(sizeof(struct Tokens));
    init_generic_token(tokens, start, end);
    tokens->type = TT_SYMBOL;
    tokens->string = str;
    return tokens;
}
#undef init_generic_token

// Index of simple token should match its corresponding enum value
static char *simple_tokens[] = {
    "if", "then", "elseif", "else", "end", "for", "to", "in", "next", "while", "function",
    "exit", "dim", "as", "string", "int", "float", "bool", "and", "or", "not", "null",
    "+", "-", "*", "/", "=", "<=", ">=", "<", ">", "(", ")", ",", "&", "_", "\n"
};

static int is_simple_token(char *input, int start, int end, enum SimpleToken *return_val)
{
    int is_equal = 1;
    for (int i = 0; i < (int) sizeof(simple_tokens); i++) {
        is_equal = 1;
        for (int j = 0; j <= (end - start) && simple_tokens[i][j] != '\0'; j++) {
            is_equal = is_equal && (simple_tokens[i][j] == input[j+start]);
        }
        if (is_equal) {
            *return_val = (enum SimpleToken) i;
            return 1;
        }
    }
    return 0;
}
/*static*/ int is_number_token(char *input, int start, int end, double *number);
static int is_comment_token(char *input, int start, int end, char *string) {
    if (input[start++] == ''') {
        while (input[start++] != '\0') /* keep on truckin */
    }
}
int is_string_token(char *input, int start, int end, char *string) {
    return input[start] == '"';
}

static struct Tokens **tokenize(char *input)
{
    struct Tokens **current;
    struct Tokens **head = current;
    enum SimpleToken st;
    double number;
    char *string;
    for (int i = 0; input[i] != '\0'; i++) {
        // ignore leading whitespace
        while (input[i] == ' ' || input[i] == '\t') {
            i++;
        }
        int start = i;
        while (input[i] != ' ' && input[i] != '\t' && input[i] != '\0') {
            i++;
        }
        if (is_simple_token(input, start, i, &st)) {
            *current = simple_token(st, start, i);
        } else if (is_number_token(input, start, i, &number)) {
            *current = number_token(number, start, i);
        } else if (input[i] == '"') {
            // error code for unterminated string
            return head;
        } else if (is_comment_token(input, start, i, string)) {
            *current = comment_token(string, start, i);
        } else if (is_string_token(input, start, i, string)) {
            *current = string_token(string, start, i);
        } else {
            *current = symbol_token(string, start, i);
        }
        *current = current->next;
    }
}

static int parse_binexpr(char *error, struct BinaryExpr *binexpr, char **input_ptr);

static void ignore_whitespace(char **input_ptr)
{
    char *input = *input_ptr;
    while (*input == ' ' || *input == '\n') input++;
    *input_ptr = input;
}

static int parse_expr(char *error, struct Expr *expr, char **input_ptr)
{
    char *input = *input_ptr;
    ignore_whitespace(&input); 

    if (isdigit(*input)) {
        // add error handling code later
        expr->type = VALUE;
        expr->value = atoi(input);
        while (!(*input == ' ' || *input == ')' || *input == '\0')) input++;
    } else if (isalpha(*input)) {
        // symbol should probably be able to contain more than alphanum in the future
        expr->type = SYMBOL;
        expr->symbol = alloc(sizeof(char) * SYMBOL_SIZE);
        for (int i = 0; i < SYMBOL_SIZE - 1 && isalnum(*input); i++) {
            expr->symbol[i] = *input;
            input++;
        }
    } else if (*input == '(') {
        expr->type = EXPRESSION;
        input++;
        expr->binexpr = alloc(sizeof(struct BinaryExpr));
        if (!parse_binexpr(error, expr->binexpr, &input)) {
            return 0;
        }
    } else {
        snprintf(error, ERR_MSG_LEN, "expected expression, got '%c'", *input);
        return 0;
    }

    ignore_whitespace(&input); 
    *input_ptr = input;
    return 1;
}

static int parse_binexpr(char *error, struct BinaryExpr *binexpr, char **input_ptr)
{
    enum BinaryOp op;
    char *input = *input_ptr;
    ignore_whitespace(&input);
    switch (*input) {
        case '+':
            op = ADDITION;
            break;
        case '-':
            op = SUBTRACTION;
            break;
        case '*':
            op = MULTIPLICATION;
            break;
        case '/':
            op = DIVISION;
            break;
        case '=':
            op = EQUAL;
            break;
        case '$':
            op = DEFINE;
            break;
        case '@':
            op = WHILE;
            break;
        // case '!=':
        //     NOT_EQUAL,
        case '<':
            op = LESS;
            break;
        case '>':
            op = GREATER;
            break;
        case '?':
            op = IF;
            break;
        default:
            snprintf(error, ERR_MSG_LEN, "expected operator but got %c", *input);
            return 0;
    }
    input++;

    struct Expr *expr1 = alloc(sizeof(struct Expr));
    if (!parse_expr(error, expr1, &input)) {
        snprintf(error, ERR_MSG_LEN, "error parsing first subexpression");
        return 0;
    } else if (op == DEFINE && expr1->type != SYMBOL) {
        snprintf(error, ERR_MSG_LEN, "error first argument of '$' must be a symbol");
        return 0;
    }

    struct Expr *expr2 = alloc(sizeof(struct Expr));
    if (!parse_expr(error, expr2, &input)) {
        snprintf(error, ERR_MSG_LEN, "error parsing second subexpression");
        return 0;
    }

    if (*input != ')') {
        snprintf(error, ERR_MSG_LEN, "unexpected end of expression");
        return 0;
    }
    input++;

    binexpr->op = op;
    binexpr->expr1 = expr1;
    binexpr->expr2 = expr2;

    *input_ptr = input;
    return 1;
}

static int parse_helper(struct Exprs *exprs, char *error, char *input)
{
    while (*input != '\0') {
        exprs->expr = alloc(sizeof(struct Expr));
        exprs->next = NULL;
        if (!parse_expr(error, exprs->expr, &input)) {
            exprs->expr = NULL;
            return 0;
        }
        if (*input != '\0') {
            exprs->next = alloc(sizeof(struct Exprs));
            exprs = exprs->next;
        }
    }
    return 1;
}

int parse(struct Exprs *exprs, char *error, char *input)
{
    int status = parse_helper(exprs, error, input);
    return status;
}

