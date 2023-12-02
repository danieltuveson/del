#include "common.h"
#include "printers.h"

void print_expr(struct Expr *expr, int depth)
{
    switch (expr->type) {
        case VALUE:
            printf("%li", expr->value);
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
                case EQUAL:
                    putchar('=');
                    break;
                case NOT_EQUAL:
                    printf("/=");
                    break;
                case LESS:
                    putchar('<');
                    break;
                case GREATER:
                    putchar('>');
                    break;
                case LEQUAL:
                    printf("<=");
                    break;
                case GEQUAL:
                    printf(">=");
                    break;
                case DEFINE:
                    printf("$");
                    break;
                default:
                    printf("***symbol not implemented***");
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

void print_instructions(void **instructions, int length)
{
    for (int i = 0; i < length; i++) {
        switch ((enum Code) instructions[i]) {
            case PUSH:
                i++;
                printf("PUSH %li\n", (long) instructions[i]);
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
            case JMP:
                printf("JMP\n");
                break;
            case RET:
                printf("RET\n");
                break;
            case DEF:
                printf("DEF\n");
                break;
            case LOAD:
                i++;
                printf("LOAD %s\n", (char *) instructions[i]);
                break;
            default:
                printf("***non-printable instruction***\n");
        }
    }
}

void print_stack(struct Stack *stack)
{
    printf("[ ");
    for (int i = 0; i < stack->offset; i++) {
        printf("%li ", (long) stack->values[i]);
    }
    printf("]\n");
}

void print_heap(struct Heap *heap)
{
    printf("[");
    while (heap->name != NULL) {
        printf(" { %s: %li }, ", heap->name, heap->value);
        heap = heap->next;
    }
    printf("]\n");
}
