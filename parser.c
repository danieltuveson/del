#include <assert.h>
#include <stdlib.h>
#include "printers.h"
#include "common.h"
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
            //free(expr->binexpr);
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
        default:
            snprintf(error, ERR_MSG_LEN, "expected operator but got %c", *input);
            return 0;
    }
    input++;

    struct Expr *expr1 = alloc(sizeof(struct Expr));
    if (!parse_expr(error, expr1, &input)) {
        //free(expr1);
        snprintf(error, ERR_MSG_LEN, "error parsing first subexpression");
        return 0;
    } else if (op == DEFINE && expr1->type != SYMBOL) {
        //free(expr1);
        snprintf(error, ERR_MSG_LEN, "error first argument of '$' must be a symbol");
        return 0;
    }

    struct Expr *expr2 = alloc(sizeof(struct Expr));
    if (!parse_expr(error, expr2, &input)) {
        //free(expr2);
        snprintf(error, ERR_MSG_LEN, "error parsing second subexpression");
        return 0;
    }

    if (*input != ')') {
        snprintf(error, ERR_MSG_LEN, "unexpected end of expression");
        //free(expr1);
        //free(expr2);
        return 0;
    }
    input++;

    binexpr->op = op;
    binexpr->expr1 = expr1;
    binexpr->expr2 = expr2;

    *input_ptr = input;
    return 1;
}

// this is not sufficient to actually delete an expression
// static void free_expr(struct Expr *expr)
// {
//     //free(expr);
// }

static int parse_helper(struct Exprs *exprs, char *error, char *input)
{
    while (*input != '\0') {
        exprs->expr = alloc(sizeof(struct Expr));
        exprs->next = NULL;
        if (!parse_expr(error, exprs->expr, &input)) {
            //free_expr(exprs->expr);
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

