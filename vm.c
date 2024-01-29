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

static struct Heap *lookup(struct Heap *heap, char *lookup_val, long *val)
{
    printf("crash\n");
    print_heap(heap);
    while (heap->name != NULL) {
        printf("heap->name: '%s'\n", heap->name);
        printf("lookup_val: '%s'\n", lookup_val);
        if (strcmp(heap->name, lookup_val) == 0) {
            if (val != NULL) {
                *val = heap->value;
            }
            return heap;
        }
    printf("doesn't happen\n");
        heap = heap->next;
    }
    return NULL;
}

static struct Heap *def(struct Stack *stack, struct Heap *heap, struct Heap *heap_current)
{
    long val = (long) pop(stack);
    char *symbol = (char *) pop(stack);
    struct Heap *heap_scratch = lookup(heap, symbol, NULL);
    if (heap_scratch) {
        heap_scratch->value = val; // update existing value
    } else {
        heap_current->value = val;
        heap_current->name = symbol;
        heap_current->next = malloc(sizeof(struct Heap));
        heap_current->next->name = NULL;
        heap_current->next->value = 0;
        heap_current->next->next = NULL;
        heap_current = heap_current->next;
    }
    return heap_current;
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

long vm_execute(struct Heap *heap, uint64_t *instructions)
{
    struct Stack stack = {0, {0}};
    struct Heap *heap_current = heap;
    // struct Heap *heap;
    // struct Locals locals;
    long ip = 0;
    long ret = 0;
    long val1, val2;
    char *symbol;
    int i = 0;
    while (1) {
        switch ((enum Code) instructions[ip]) {
            case PUSH:
                ip++;
                push(&stack, instructions[ip]);
                break;
            case PUSH_HEAP:
                break;
            case AND: eval_binary_op(&stack, val1, val2, &&, long); break;
            case OR: eval_binary_op(&stack, val1, val2, ||, long); break;
            case ADD: eval_binary_op(&stack, val1, val2, +, long); break;
            case SUB: eval_binary_op(&stack, val1, val2, -, long); break;
            case MUL: eval_binary_op(&stack, val1, val2, *, long); break;
            case DIV: eval_binary_op(&stack, val2, val1, /, long); break;
            case EQ: eval_binary_op(&stack, val1, val2, ==, long); break;
            case NEQ: eval_binary_op(&stack, val1, val2, !=, long); break;
            case LTE: eval_binary_op(&stack, val1, val2, <=, long); break;
            case GTE: eval_binary_op(&stack, val1, val2, >=, long); break;
            case LT: eval_binary_op(&stack, val1, val2, <, long); break;
            case GT: eval_binary_op(&stack, val1, val2, >, long); break;
            case UNARY_PLUS: val1 = (long) pop(&stack);
                push(&stack, (uint64_t) val1); break;
            case UNARY_MINUS: val1 = (long) pop(&stack);
                push(&stack, (uint64_t) (-1 * val1)); break;
            case SET:
                assert("error SET is not yet implemented\n" && 0);
                break;
            //     val1 = (long) pop(&stack);
            //     symbol = (char *) pop(&stack);
            //     heap_scratch = lookup(heap, symbol, &val2);
            //     heap_scratch->value = val1; // update existing value
            case DEF:
                heap_current = def(&stack, heap, heap_current);
                // heap_current = def(&stack, heap);
                print_heap(heap);
                break;
            case LOAD:
                ip++;
                symbol = (char *) instructions[ip];
                if (lookup(heap, symbol, &val1)) {
                    push(&stack, (uint64_t) val1);
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
                    //printf("jne'ing to %li!\n", ip);
                    ip--; // reverse the effects of the ip++ below
                } else {
                    pop(&stack);
                }
                break;
            case JMP:
                ip = (long) pop(&stack);
                //printf("jumping to %li!\n", ip);
                ip--; // reverse the effects of the ip++ below
                break;
            case POP:
                ret = (long) pop(&stack);
                break;
            case RET:
                goto exit_loop;
            case CALL:
                symbol = (char *) pop(&stack);
                if (strcmp(symbol, "print") == 0) {
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

