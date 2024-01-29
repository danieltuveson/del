#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"

static inline void push(struct Stack *stack, uint64_t value)
{
    stack->values[stack->offset++] = value;
}

static inline uint64_t pop(struct Stack *stack)
{
    return stack->values[--stack->offset];
}

#define eval_binary_op(stack_ptr, v1, v2, op, type) \
    do { \
        v1 = (type) pop(stack_ptr); \
        v2 = (type) pop(stack_ptr); \
        push(stack_ptr, (uint64_t) (v2 op v1)); } while (0)


// struct Locals {
//     Symbol names[HEAP_SIZE];
//     uint64_t values[HEAP_SIZE];
//     int count;
//     int stack_depth;
// };

static int lookup(struct Locals *locals, Symbol lookup_val, uint64_t *val)
{
    for (int i = 0; i < locals->count; i++) {
        if (lookup_val == locals->names[i]) {
            *val = locals->values[i];
            return 1;
        }
    }
    return 0;
}

static void def(struct Stack *stack, struct Locals *locals)
{
    uint64_t val = (uint64_t) pop(stack);
    Symbol symbol = (Symbol) pop(stack);
    int i;
    for (i = 0; i < locals->count; i++) {
        if (symbol == locals->names[i]) {
            locals->values[i] = val;
            return;
        }
    }
    locals->count++;
    locals->names[i] = symbol;
    locals->values[i] = val;
}

// static char *get_string(struct Heap *heap, uint64_t heap_loc)
// {
// }
// 
// static struct Locals *def(struct Stack *stack, struct Locals *locals)
// {
//     // struct Locals {
//     // char *names[HEAP_SIZE];
//     // uint64_t values[HEAP_SIZE];
//     // int count;
//     // int stack_depth;
//     int32_t val = (int32_t) pop(stack);
//     uint64_t heap_loc = (uint64_t) pop(stack);
//     char *str = get_string(struct Heap *heap, heap_loc);
//     locals->names = "";
//     locals->count++;
// }

long vm_execute(struct Locals *locals, uint64_t *instructions)
{
    struct Stack stack = {0, {0}};
    uint64_t ip = 0;
    uint64_t ret = 0;
    uint64_t val1, val2;
    Symbol symbol;
    int i = 0;
    while (1) {
        switch ((enum Code) instructions[ip]) {
            case PUSH:
                ip++;
                push(&stack, instructions[ip]);
                break;
            case PUSH_HEAP:
                break;
            case AND: eval_binary_op(&stack, val1, val2, &&, uint64_t); break;
            case OR: eval_binary_op(&stack, val1, val2, ||, uint64_t); break;
            case ADD: eval_binary_op(&stack, val1, val2, +, uint64_t); break;
            case SUB: eval_binary_op(&stack, val1, val2, -, uint64_t); break;
            case MUL: eval_binary_op(&stack, val1, val2, *, uint64_t); break;
            case DIV: eval_binary_op(&stack, val2, val1, /, uint64_t); break;
            case EQ: eval_binary_op(&stack, val1, val2, ==, uint64_t); break;
            case NEQ: eval_binary_op(&stack, val1, val2, !=, uint64_t); break;
            case LTE: eval_binary_op(&stack, val1, val2, <=, uint64_t); break;
            case GTE: eval_binary_op(&stack, val1, val2, >=, uint64_t); break;
            case LT: eval_binary_op(&stack, val1, val2, <, uint64_t); break;
            case GT: eval_binary_op(&stack, val1, val2, >, uint64_t); break;
            case UNARY_PLUS: val1 = (uint64_t) pop(&stack);
                push(&stack, (uint64_t) val1); break;
            case UNARY_MINUS: val1 = (uint64_t) pop(&stack);
                push(&stack, (uint64_t) (-1 * val1)); break;
            case SET:
                assert("error SET is not yet implemented\n" && 0);
                break;
            //     val1 = (long) pop(&stack);
            //     symbol = (char *) pop(&stack);
            //     heap_scratch = lookup(heap, symbol, &val2);
            //     heap_scratch->value = val1; // update existing value
            case DEF:
                def(&stack, locals);
                print_locals(locals);
                break;
            case LOAD:
                ip++;
                symbol = (Symbol) instructions[ip];
                if (lookup(locals, symbol, &val1)) {
                    push(&stack, (uint64_t) val1);
                } else {
                    printf("variable '%s' is undefined\n", lookup_symbol(symbol));
                    goto exit_loop;
                }
                break;
            case JE:
                break;
            case JNE:
                val1 = (uint64_t) pop(&stack);
                if (!val1) {
                    ip = (uint64_t) pop(&stack);
                    //printf("jne'ing to %li!\n", ip);
                    ip--; // reverse the effects of the ip++ below
                } else {
                    pop(&stack);
                }
                break;
            case JMP:
                ip = (uint64_t) pop(&stack);
                //printf("jumping to %li!\n", ip);
                ip--; // reverse the effects of the ip++ below
                break;
            case POP:
                ret = (uint64_t) pop(&stack);
                break;
            case RET:
                goto exit_loop;
            case CALL:
                symbol = (Symbol) pop(&stack);
                if (strcmp(lookup_symbol(symbol), "print") == 0) {
                    printf("%s\n", (char *) pop(&stack));
                } else {
                    assert("unknown function encountered\n" && 0);
                }
                break;
            default:
                printf("unknown instruction encountered: '%llu'", instructions[ip]);
                break;
        }
        ip++;
        i++;
        print_stack(&stack);
        if (i > 20) {
            printf("infinite loop detected, ending execution\n");
            break;
        }
    }
exit_loop:
    // print_stack(&stack);
    return ret;
}

#undef eval_binary_op

