#include "common.h"
#include "parser.h"


static void ignore_whitespace(char **input_ptr)
{
    char *input = *input_ptr;
    while (*input == ' ' || *input == '\n') input++;
    *input_ptr = input;
}

static int parse_binexpr(struct Parser *parser, struct BinaryExpr *binexpr, char **input_ptr);

static int parse_expr(struct Parser *parser, struct Expr *expr, char **input_ptr)
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
        expr->symbol = calloc(sizeof(char), SYMBOL_SIZE);
        for (int i = 0; i < SYMBOL_SIZE - 1 && isalnum(*input); i++) {
            expr->symbol[i] = *input;
            input++;
        }
    } else if (*input == '(') {
        expr->type = EXPRESSION;
        input++;
        expr->binexpr = malloc(sizeof(struct BinaryExpr));
        if (!parse_binexpr(parser, expr->binexpr, &input)) {
            free(expr->binexpr);
            return 0;
        }
    } else {
        snprintf(parser->error, ERR_MSG_LEN, "expected expression, got '%c'", *input);
        return 0;
    }

    ignore_whitespace(&input); 
    *input_ptr = input;
    return 1;
}

static int parse_binexpr(struct Parser *parser,
        struct BinaryExpr *binexpr, char **input_ptr)
{
    enum BinaryOp op;
    char *input = *input_ptr;
    // if (*input != '(') {
    //     sprintf(parser->error, "unexpected character %c", *input);
    //     return 0;
    // }
    // input++;
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
        default:
            snprintf(parser->error, ERR_MSG_LEN, "expected operator but got %c", *input);
            return 0;
    }
    input++;

    struct Expr *expr1 = malloc(sizeof(struct Expr));
    if (!parse_expr(parser, expr1, &input)) {
        free(expr1);
        snprintf(parser->error, ERR_MSG_LEN, "error parsing first subexpression");
        return 0;
    } else if (op == DEFINE && expr1->type != SYMBOL) {
        free(expr1);
        snprintf(parser->error, ERR_MSG_LEN, "error first argument of '$' must be a symbol");
        return 0;
    }

    struct Expr *expr2 = malloc(sizeof(struct Expr));
    if (!parse_expr(parser, expr2, &input)) {
        free(expr2);
        snprintf(parser->error, ERR_MSG_LEN, "error parsing second subexpression");
        return 0;
    }

    if (*input != ')') {
        snprintf(parser->error, ERR_MSG_LEN, "unexpected end of expression");
        free(expr1);
        free(expr2);
        return 0;
    }
    input++;

    binexpr->op = op;
    binexpr->expr1 = expr1;
    binexpr->expr2 = expr2;

    *input_ptr = input;
    return 1;
}

int parse(struct Parser *parser, char *input)
{
    int is_success = parse_expr(parser, parser->expr, &input);
    if (!is_success) {
        return 0;
    }
    return 1;
}
