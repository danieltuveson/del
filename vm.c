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

static inline void swap(struct Stack *stack)
{
    uint64_t val1, val2;
    val1 = pop(stack);
    val2 = pop(stack);
    push(stack, val1);
    push(stack, val2);
//     uint64_t swp = stack->values[stack->offset];
//     stack->values[stack->offset] = stack->values[stack->offset-1];
//     stack->values[stack->offset-1] = swp;
}

#define eval_binary_op(stack_ptr, v1, v2, op, type) \
    do { \
        v1 = (type) pop(stack_ptr); \
        v2 = (type) pop(stack_ptr); \
        push(stack_ptr, (uint64_t) (v2 op v1)); } while (0)

static void uint64_as_string(uint64_t value, char *str, int start)
{
    for (int i = 0; i < 8; i++) {
        str[i + start] = (char) (value >> (8 * i));
    }
    str[8 + start] = '\0';
    printf("decoded as string '%s'\n", str);
}

/* Pops values from the stack and pushes them onto the heap */
static inline void push_heap(struct Heap *heap, struct Stack *stack)
{
    uint64_t value, count, location;
    value = 0;
    count = pop(stack);
    location = heap->offset;
    heap->values[heap->offset++] = count;
    printf("count: %llu\n", count);
    printf("location: %llu\n", location);
    for (uint64_t i = 0; i < count; i++) {
        value = pop(stack);
        // char str[9] = {0};
        // uint64_as_string(value, str, 0);
        // printf("value: %llu\n", value);
        heap->values[heap->offset++] = value;
    }
    push(stack, location);
    heap->objcount++;
}

/* Get value from the heap and push it onto the stack */
static inline void get_heap(struct Heap *heap, struct Stack *stack)
{
    uint64_t count, location;
    location = pop(stack);
    count = heap->values[location];
    for (uint64_t i = count + location - 1; i >= location; i--) {
        push(stack, heap->values[i]);
    }
}

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
    Symbol symbol = (Symbol) pop(stack);
    uint64_t val = (uint64_t) pop(stack);
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

// static char *load_string(struct Stack *stack, struct Heap *heap)
// {
//     uint64_t packed = 0;
//     uint64_t i = 0;
//     uint64_t tmp;
//     for (; string[i] != '\0'; i++) {
//         tmp = (uint64_t) string[i];
//         switch ((i + 1) % 8) {
//             case 0:
//                 packed = packed | (tmp << 56);
//                 load(PUSH_HEAP);
//                 load(packed);
//                 break;
//             case 1: packed = tmp;                  break;
//             case 2: packed = packed | (tmp << 8);  break;
//             case 3: packed = packed | (tmp << 16); break;
//             case 4: packed = packed | (tmp << 24); break;
//             case 5: packed = packed | (tmp << 32); break;
//             case 6: packed = packed | (tmp << 40); break;
//             case 7: packed = packed | (tmp << 48); break;
//         }
//     }
//     // If string is not multiple of 8 bytes, push the remainder
//     if ((i + 1) % 8 != 0) {
//         load(PUSH_HEAP);
//         load(packed);
//     }
//     return offset;
// }

long vm_execute(uint64_t *instructions)
{
    struct Locals locals = { {0}, {0}, 0, 0 };
    struct Stack stack = {0, {0}};
    struct Heap heap = {0, 0, {0}};
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
                push_heap(&heap, &stack);
                print_heap(&heap);
                break;
            /* Grotesque lump of binary operators. Boring! */
            case AND: eval_binary_op(&stack, val1, val2, &&, int64_t); break;
            case OR:  eval_binary_op(&stack, val1, val2, ||, int64_t); break;
            case ADD: eval_binary_op(&stack, val1, val2, +, int64_t);  break;
            case SUB: eval_binary_op(&stack, val1, val2, -, int64_t);  break;
            case MUL: eval_binary_op(&stack, val1, val2, *, int64_t);  break;
            case DIV: eval_binary_op(&stack, val2, val1, /, int64_t);  break;
            case EQ:  eval_binary_op(&stack, val1, val2, ==, int64_t); break;
            case NEQ: eval_binary_op(&stack, val1, val2, !=, int64_t); break;
            case LTE: eval_binary_op(&stack, val1, val2, <=, int64_t); break;
            case GTE: eval_binary_op(&stack, val1, val2, >=, int64_t); break;
            case LT:  eval_binary_op(&stack, val1, val2, <, int64_t);  break;
            case GT:  eval_binary_op(&stack, val1, val2, >, int64_t);  break;
            case UNARY_PLUS: val1 = (int64_t) pop(&stack);
                push(&stack, (uint64_t) val1); break;
            case UNARY_MINUS: val1 = (int64_t) pop(&stack);
                push(&stack, (uint64_t) (-1 * val1)); break;
            case SET:
                assert("error SET is not yet implemented\n" && 0);
                break;
            case DEF:
                def(&stack, &locals);
                print_locals(&locals);
                break;
            case LOAD:
                ip++;
                symbol = (Symbol) instructions[ip];
                if (lookup(&locals, symbol, &val1)) {
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
                    ip--; // reverse the effects of the ip++ below
                } else {
                    pop(&stack);
                }
                break;
            case JMP:
                ip = (uint64_t) pop(&stack);
                ip--; // reverse the effects of the ip++ below
                break;
            case POP:
                ret = (uint64_t) pop(&stack);
                break;
            case GET_HEAP:
                get_heap(&heap, &stack);
                break;
            case EXIT:
                goto exit_loop;
            case CALL:
                symbol = (Symbol) pop(&stack);
                if (strcmp(lookup_symbol(symbol), "print") == 0) {
                    printf("%s\n", (char *) pop(&stack));
                } else {
                    assert("unknown function encountered\n" && 0);
                }
                break;
            case SWAP:
                swap(&stack);
                break;
            default:
                printf("unknown instruction encountered: '%llu'", instructions[ip]);
                break;
        }
        ip++;
        i++;
        print_stack(&stack);
        if (i > 30) {
            printf("infinite loop detected, ending execution\n");
            break;
        }
    }
exit_loop:
    print_locals(&locals);
    return ret;
}

#undef eval_binary_op

