#include "common.h"
#include "compiler.h"

#define compile_binary_op(instructions, expr1, expr2, offset, op) \
    do { \
        offset = compile_expression(instructions, expr1, offset++); \
        offset = compile_expression(instructions, expr2, offset++); \
        instructions[offset++] = (void *) op; } while (0)

static int compile_expression(void **instructions, struct Expr *expr, int offset)
{
    struct Expr *expr1, *expr2;
    switch (expr->type) {
        case VALUE:
            instructions[offset++] = (void *) PUSH;
            instructions[offset++] = (void *) expr->value;
            break;
        case SYMBOL:
            instructions[offset++] = (void *) LOAD;
            instructions[offset++] = (void *) expr->symbol;
            break;
        case EXPRESSION:
            expr1 = expr->binexpr->expr1;
            expr2 = expr->binexpr->expr2;
            switch (expr->binexpr->op) {
                case ADDITION:
                    compile_binary_op(instructions, expr1, expr2, offset, ADD);
                    break;
                case SUBTRACTION:
                    compile_binary_op(instructions, expr1, expr2, offset, SUB);
                    break;
                case MULTIPLICATION:
                    compile_binary_op(instructions, expr1, expr2, offset, MUL);
                    break;
                case DIVISION:
                    compile_binary_op(instructions, expr1, expr2, offset, DIV);
                    break;
                case EQUAL:
                    compile_binary_op(instructions, expr1, expr2, offset, EQ);
                    break;
                case DEFINE:
                    instructions[offset++] = (void *) PUSH;
                    instructions[offset++] = (void *) expr1->symbol;
                    offset = compile_expression(instructions, expr2, offset++);
                    instructions[offset++] = (void *) DEF;
                    break;
                default:
                    printf("Error cannot compile expression\n");
                    break;
            }
            break;
    }
    return offset;
}

int compile(void **instructions, struct Exprs *exprs, int offset)
{
    struct Expr *expr;
    do {
        expr = exprs->expr;
        offset = compile_expression(instructions, expr, offset);
        // Discard top level expression from the stack since it's not part of a subexpression
        if (expr->type == EXPRESSION) {
            instructions[offset++] = (void *) POP;
        }
        instructions[offset] = (void *) RET;
        exprs = exprs->next;
    } while (exprs != NULL);
    return offset;
}

