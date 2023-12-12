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

static int lookup(struct Heap *heap, char *lookup_val, long *val)
{
    while (heap->name != NULL) {
        if (strcmp(heap->name, lookup_val) == 0) {
            *val = heap->value;
            return 1;
        }
        heap = heap->next;
    }
    return 0;
}

long vm_execute(void **instructions)
{
    struct Stack stack = {0, {0}};
    struct Heap heap = { NULL, 0, NULL };
    struct Heap *heap_val = &heap;
    int i = 0;
    long ret = 0;
    long val1, val2;
    char *symbol;
    while (1) {
        print_stack(&stack);
        switch ((enum Code) instructions[i]) {
            case PUSH:
                i++;
                push(&stack, instructions[i]);
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
            case DEF:
                heap_val->value = (long) pop(&stack);
                heap_val->name = (char *) pop(&stack);
                heap_val->next = malloc(sizeof(struct Heap));
                push(&stack, (void *) heap_val->value);
                heap_val->next->name = NULL;
                heap_val->next->value = 0;
                heap_val->next->next = NULL;
                heap_val = heap_val->next;
                break;
            case LOAD:
                i++;
                symbol = (char *) instructions[i];
                if (lookup(&heap, symbol, &val1)) {
                    push(&stack, (void *) val1);
                } else {
                    printf("variable '%s' is undefined\n", symbol);
                    goto exit_loop;
                }
                break;
            // EQCMP,
            // NEQCMP,
            // LCMP,
            // GCMP,
            case JE:
                break;
            case JMP:
                break;
            case POP:
                ret = (long) pop(&stack);
                break;
            case RET:
                goto exit_loop;
            default:
                printf("unknown instruction encountered");
                break;
        }
        i++;
    }
exit_loop:
    print_stack(&stack);
    print_heap(&heap);
    return ret;
}

