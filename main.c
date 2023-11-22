#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define STACK_SIZE  10
#define SYMBOL_SIZE 24

#define push(stack, value) (stack)->values[(stack)->offset++] = (value)
#define pop(stack) (stack)->values[--(stack)->offset]


struct Stack {
    int offset;
    int values[STACK_SIZE];
};

enum Code {
    PUSH,
    ADD,
    SUB,
    MUL,
    DIV,
    JE,
    JNE,
    JMP,
    RET
};

enum ExprType {
    VALUE,
    SYMBOL,
    EXPRESSION
};

struct BinaryExpr;

struct Expr {
    enum ExprType type;
    union {
        int value;
        char *symbol;
        struct BinaryExpr *binexpr;
    };
};

enum BinaryOp {
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    DIVISION
};

struct BinaryExpr {
    enum BinaryOp op;
    struct Expr *expr1;
    struct Expr *expr2;
};

struct Parser {
    struct Expr *expr;
    char *error;
};

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
            goto err;
        }
    } else {
        goto err;
    }

    ignore_whitespace(&input); 
    *input_ptr = input;
    return 1;

err:
    sprintf(parser->error, "expected expression");
    return 0;
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
        default:
            sprintf(parser->error, "expected operator but got %c", *input);
            return 0;
    }
    input++;

    struct Expr *expr1 = malloc(sizeof(struct Expr));
    if (!parse_expr(parser, expr1, &input)) {
        free(expr1);
        sprintf(parser->error, "error parsing subexpression");
        return 0;
    }

    struct Expr *expr2 = malloc(sizeof(struct Expr));
    if (!parse_expr(parser, expr2, &input)) {
        free(expr2);
        sprintf(parser->error, "error parsing subexpression");
        return 0;
    }

    if (*input != ')') {
        sprintf(parser->error, "unexpected end of expression");
        free(expr1);
        free(expr2);
        return 0;
    }
    binexpr->op = op;
    binexpr->expr1 = expr1;
    binexpr->expr2 = expr2;

    *input_ptr = input;
    return 1;
}

static int parse(struct Parser *parser, char *input)
{
    int is_success = parse_expr(parser, parser->expr, &input);
    if (!is_success) {
        return 0;
    }
    return 1;
}

static void print_expr(struct Expr *expr, int depth)
{
    switch (expr->type) {
        case VALUE:
            printf("%i", expr->value);
            break;
        case SYMBOL:
            printf("%s", expr->symbol);
            break;
        case EXPRESSION:
            putchar('(');
            switch (expr->binexpr->op) {
                case ADDITION:
                    putchar('+');
                    break;
                case SUBTRACTION:
                    putchar('-');
                    break;
                case MULTIPLICATION:
                    putchar('*');
                    break;
                case DIVISION:
                    putchar('/');
                    break;
            }
            depth++; // currently not used, might use later for indentation
            putchar(' ');
            print_expr(expr->binexpr->expr1, depth);
            putchar(' ');
            print_expr(expr->binexpr->expr2, depth);
            putchar(')');
            break;
    }
}

static void print_instructions(int *instructions, int length)
{
    for (int i = 0; i < length; i++) {
        switch ((enum Code) instructions[i]) {
            case PUSH:
                i++;
                printf("PUSH %i\n", instructions[i]);
                break;
            case ADD:
                printf("ADD\n");
                break;
            case SUB:
                printf("SUB\n");
                break;
            case MUL:
                printf("MUL\n");
                break;
            case DIV:
                printf("DIV\n");
                break;
            case JE:
                printf("JE\n");
                break;
            case JNE:
                printf("JNE\n");
                break;
            case JMP:
                printf("JMP\n");
                break;
            case RET:
                printf("RET\n");
                break;
        }
    }
}

static int compile(int *instructions, struct Expr *expr, int offset)
{
    switch (expr->type) {
        case VALUE:
            instructions[offset++] = PUSH;
            instructions[offset++] = expr->value;
            break;
        case SYMBOL:
            // expr->symbol;
            printf("cannot use variables - not implemented\n");
            exit(1);
            // break;
        case EXPRESSION:
            switch (expr->binexpr->op) {
                case ADDITION:
                    offset = compile(instructions, expr->binexpr->expr1, offset++);
                    offset = compile(instructions, expr->binexpr->expr2, offset++);
                    instructions[offset++] = ADD;
                    break;
                case SUBTRACTION:
                    offset = compile(instructions, expr->binexpr->expr1, offset++);
                    offset = compile(instructions, expr->binexpr->expr2, offset++);
                    instructions[offset++] = SUB;
                    break;
                case MULTIPLICATION:
                    offset = compile(instructions, expr->binexpr->expr1, offset++);
                    offset = compile(instructions, expr->binexpr->expr2, offset++);
                    instructions[offset++] = MUL;
                    break;
                case DIVISION:
                    offset = compile(instructions, expr->binexpr->expr1, offset++);
                    offset = compile(instructions, expr->binexpr->expr2, offset++);
                    instructions[offset++] = DIV;
                    break;
            }
            break;
    }
    return offset;
}

static void print_stack(struct Stack *stack)
{
    printf("[ ");
    for (int i = 0; i < stack->offset; i++) {
        printf("%i ", stack->values[i]);
    }
    printf("]\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Expected input expression\n");
        return -1;
    }

    struct Parser parser;
    struct Expr expr; 
    parser.expr = &expr;
    parser.error = calloc(sizeof(char), 100); // probably enough characters

    int is_success = parse(&parser, argv[1]);
    if (!is_success) {
        printf("Error parsing expression: %s\n", parser.error);
        return -1;
    }
    printf("expression: ");
    print_expr(&expr, 0);
    putchar('\n');

    struct Stack stack = { 0, {0} };
    int instructions[100];
    int offset = compile(instructions, &expr, 0);
    instructions[offset] = RET;
    print_instructions(instructions, offset+1);
    // int instructions[] = {
    //     PUSH, 4,
    //     PUSH, 3,
    //     ADD,
    //     PUSH, 3,
    //     MUL,
    //     RET
    // };

    int ret = 0;
    int i = 0;
    int val1, val2;
    while (1) {
        print_stack(&stack);
        switch ((enum Code) instructions[i]) {
            case PUSH:
                i++;
                push(&stack, instructions[i]);
                break;
            case ADD:
                val1 = pop(&stack);
                val2 = pop(&stack);
                push(&stack, val1 + val2);
                break;
            case SUB:
                val1 = pop(&stack);
                val2 = pop(&stack);
                push(&stack, val2 - val1);
                break;
            case MUL:
                val1 = pop(&stack);
                val2 = pop(&stack);
                push(&stack, val1 * val2);
                break;
            case DIV:
                val1 = pop(&stack);
                val2 = pop(&stack);
                push(&stack, val2 / val1);
                break;
            case JE:
                break;
            case JNE:
                break;
            case JMP:
                break;
            case RET:
                ret = pop(&stack);
                goto exit_loop;
        }
        i++;
        // if (i > 2000) break; // just in case
    }
exit_loop:
    printf("ret: %i\n", ret);
    return 0;
}

