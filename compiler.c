#include "common.h"
#include "compiler.h"

#define load(op) instructions[offset++] = (void *) op

static int compile_expression(void **instructions, struct Expr *expr, int offset);

static inline void compile_binary_op_helper(void **instructions, struct Expr *expr1, struct Expr *expr2, int *offset_ptr, void *op) {
    int offset = *offset_ptr;
    offset = compile_expression(instructions, expr1, offset++);
    offset = compile_expression(instructions, expr2, offset++);
    load(op);
    *offset_ptr = offset;
}
#define compile_binary_op(instructions, expr1, expr2, offset, op) compile_binary_op_helper(instructions, expr1, expr2, &offset, (void *) op)

static int compile_expression(void **instructions, struct Expr *expr, int offset)
{
    struct Expr *expr1, *expr2;
    int top_of_loop = 0;
    int old_offset = 0;
    switch (expr->type) {
        case VALUE:
            load(PUSH);
            load(expr->value);
            break;
        case SYMBOL:
            load(LOAD);
            load(expr->symbol);
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
                case LESS:
                    compile_binary_op(instructions, expr1, expr2, offset, LT);
                    break;
                case GREATER:
                    compile_binary_op(instructions, expr1, expr2, offset, GT);
                    break;
                case DEFINE:
                    load(PUSH);
                    load(expr1->symbol);
                    offset = compile_expression(instructions, expr2, offset++);
                    load(DEF);
                    break;
                case WHILE:
                    top_of_loop = offset;
                    load(PUSH);
                    old_offset = offset++;
                    offset = compile_expression(instructions, expr1, offset++);
                    load(JNE);
                    offset = compile_expression(instructions, expr2, offset++);
                    load(PUSH);
                    load(top_of_loop);
                    load(JMP);

                    instructions[old_offset] = (void *) offset; // set JNE jump to go to end of loop
                    break;
                case IF:
                    top_of_loop = offset;
                    load(PUSH);
                    old_offset = offset++;
                    offset = compile_expression(instructions, expr1, offset++);
                    load(JNE);
                    offset = compile_expression(instructions, expr2, offset++);

                    instructions[old_offset] = (void *) offset; // set JNE jump to go to after if statement
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
        // (unless it's a statement that returns nothing)
        if (expr->type == EXPRESSION && (expr->binexpr->op != WHILE && expr->binexpr->op != IF)) {
            load(POP);
        }
        instructions[offset] = (void *) RET;
        exprs = exprs->next;
    } while (exprs != NULL);
    return offset;
}

