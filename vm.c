#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"

static inline void push(struct Stack *stack, void *value)
{
    stack->values[stack->offset++] = value;
}

#define top(stack_ptr) (stack_ptr)->values[(stack_ptr)->offset]
#define pop(stack_ptr) (stack_ptr)->values[--(stack_ptr)->offset]
#define eval_binary_op(stack_ptr, v1, v2, op) \
    do { \
        v1 = (long) pop(stack_ptr); \
        v2 = (long) pop(stack_ptr); \
        push(stack_ptr, (void *) (v2 op v1)); } while (0)

static struct Heap *lookup(struct Heap *heap, char *lookup_val, long *val)
{
    while (heap->name != NULL) {
        if (strcmp(heap->name, lookup_val) == 0) {
            *val = heap->value;
            return heap;
        }
        heap = heap->next;
    }
    return NULL;
}

long vm_execute(struct Heap *heap, void **instructions)
{
    struct Stack stack = {0, {0}};
    struct Heap *heap_val = heap;
    struct Heap *heap_scratch = NULL;
    long ip = 0;
    long ret = 0;
    long val1, val2;
    char *symbol;
    int i = 0;
    while (1) {
        print_stack(&stack);
        switch ((enum Code) instructions[ip]) {
            case PUSH:
                ip++;
                push(&stack, instructions[ip]);
                break;
            case ADD:
                eval_binary_op(&stack, val1, val2, +);
                break;
            case SUB:
                eval_binary_op(&stack, val1, val2, -);
                break;
            case MUL:
                eval_binary_op(&stack, val1, val2, *);
                break;
            case DIV:
                eval_binary_op(&stack, val2, val1, /);
                break;
            case EQ:
                eval_binary_op(&stack, val1, val2, ==);
                break;
            case NEQ:
                eval_binary_op(&stack, val1, val2, !=);
                break;
            case LT:
                eval_binary_op(&stack, val1, val2, <);
                break;
            case GT:
                eval_binary_op(&stack, val1, val2, >);
                break;
            case DEF:
                val1 = (long) pop(&stack);
                symbol = (char *) pop(&stack);
                heap_scratch = lookup(heap, symbol, &val2);
                if (heap_scratch) {
                    heap_scratch->value = val1; // update existing value
                } else {
                    heap_val->value = val1;
                    heap_val->name = symbol;
                    heap_val->next = malloc(sizeof(struct Heap));
                    push(&stack, (void *) heap_val->value);
                    heap_val->next->name = NULL;
                    heap_val->next->value = 0;
                    heap_val->next->next = NULL;
                    heap_val = heap_val->next;
                }
                break;
            case LOAD:
        print_heap(heap);
                ip++;
                symbol = (char *) instructions[ip];
                if (lookup(heap, symbol, &val1)) {
                    push(&stack, (void *) val1);
                } else {
                    printf("variable '%s' is undefined\n", symbol);
                    goto exit_loop;
                }
                break;
            case JE:
                break;
            case JNE:
                val1 = (long) pop(&stack);
                if (!val1) {
                    ip = (long) pop(&stack);
                    printf("jne'ing to %li!\n", ip);
                    ip--; // reverse the effects of the ip++ below
                } {
                    pop(&stack);
                }
                break;
            case JMP:
                ip = (long) pop(&stack);
                printf("jumping to %li!\n", ip);
                ip--; // reverse the effects of the ip++ below
                break;
            case POP:
                ret = (long) pop(&stack);
                break;
            case RET:
                goto exit_loop;
            default:
                printf("unknown instruction encountered: '%p'", instructions[ip]);
                break;
        }
        ip++;
        i++;
        if (i > 100) {
            printf("infinite loop detected, ending execution\n");
            break;
        }
    }
exit_loop:
    // print_stack(&stack);
    return ret;
}

#undef top
#undef pop
#undef eval_binary_op
