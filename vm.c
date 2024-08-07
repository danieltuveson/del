#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"

typedef uint64_t HeapPointer;

static inline void set_count(HeapPointer *ptr, uint64_t count)
{
    assert(count <= UINT16_MAX);
    *ptr = *ptr | (count << COUNT_OFFSET);
}

static inline uint64_t get_count(HeapPointer ptr)
{
    return (ptr & COUNT_MASK) >> COUNT_OFFSET;
}

static inline uint64_t get_location(HeapPointer ptr)
{
    return ptr & LOCATION_MASK;
}

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

// static void uint64_as_string(uint64_t value, char *str, int start)
// {
//     for (int i = 0; i < 8; i++) {
//         str[i + start] = (char) (value >> (8 * i));
//     }
//     str[8 + start] = '\0';
//     printf("decoded as string '%s'\n", str);
// }

/* Pops values from the stack and pushes them onto the heap */
static inline void push_heap(struct Heap *heap, struct Stack *stack)
{
    uint64_t value, count, ptr;
    value = 0;
    count = pop(stack);
    ptr = heap->offset;
    set_count(&ptr, count);
    // heap->values[heap->offset++] = count;
    // printf("count: %" PRIu64 "\n", count);
    // printf("location: %" PRIu64 "\n", location);
    for (uint64_t i = 0; i < count; i++) {
        value = pop(stack);
        // char str[9] = {0};
        // uint64_as_string(value, str, 0);
        // printf("value: %" PRIu64 "\n", value);
        heap->values[heap->offset++] = value;
    }
    // Store count in bits before location
    push(stack, ptr);
    heap->objcount++;
}

/* Get value from the heap and push it onto the stack */
// static inline void get_heap(struct Heap *heap, struct Stack *stack)
// {
//     uint64_t ptr, count, location;
//     ptr = pop(stack);
//     location = get_location(ptr);
//     count = get_count(ptr);
//     for (uint64_t i = count + location - 1; i >= location; i--) {
//         push(stack, heap->values[i]);
//     }
// }

static inline void get_heap(struct Heap *heap, struct Stack *stack)
{
    uint64_t index, ptr, location;
    index = pop(stack);
    ptr = pop(stack);
    location = get_location(ptr);
    push(stack, heap->values[location + index]);
}

static inline void set_heap(struct Heap *heap, struct Stack *stack)
{
    printf("setting heap\n");
    uint64_t value, index, ptr, location;
    index = pop(stack);
    ptr = pop(stack);
    value = pop(stack);
    location = get_location(ptr);
    heap->values[location + index] = value;
    printf("value: %" PRIu64 ", ptr: %" PRIu64 ", index: %" PRIu64 ", location: %" PRIu64 "\n", 
           value, ptr, index, location);
    printf("done setting heap\n");
}

static inline void stack_frame_enter(struct StackFrames *sfs)
{
    sfs->frame_offsets[sfs->frame_offsets_index++] = sfs->index;
}

static inline void stack_frame_exit(struct StackFrames *sfs)
{
    sfs->frame_offsets_index--;
    sfs->index = sfs->frame_offsets[sfs->frame_offsets_index];
}

static inline size_t stack_frame_offset(struct StackFrames *sfs)
{
    return sfs->frame_offsets[sfs->frame_offsets_index-1];
}

static bool lookup_local(struct StackFrames *sfs, Symbol lookup_val, uint64_t *val)
{
    for (size_t i = stack_frame_offset(sfs); i < sfs->index; i++) {
        if (lookup_val == sfs->names[i]) {
            *val = sfs->values[i];
            return true;
        }
    }
    return false;
}

static void def(struct Stack *stack, struct StackFrames *sfs)
{
    Symbol symbol = (Symbol) pop(stack);
    uint64_t val = (uint64_t) pop(stack);
    size_t i = stack_frame_offset(sfs);
    while (i < sfs->index) {
        if (symbol == sfs->names[i]) {
            sfs->values[i] = val;
            return;
        }
        i++;
    }
    sfs->names[i] = symbol;
    sfs->values[i] = val;
    sfs->index++;
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

int vm_execute(uint64_t *instructions)
{
    // struct Locals locals = { {0}, {0}, 0, 0 };
    struct StackFrames sfs = {0, {0}, {0}, 0, {0}};
    struct Stack stack = {0, {0}};
    struct Heap heap = {0, 0, {0}};
    uint64_t ip = 0;
    uint64_t ret = 0;
    uint64_t val1 = 0, val2 = 0;
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
                // print_heap(&heap);
                break;
            case SET_HEAP:
                set_heap(&heap, &stack);
                break;
            /* Grotesque lump of binary operators. Boring! */
            case AND: eval_binary_op(&stack, val1, val2, &&, int64_t); break;
            case OR:  eval_binary_op(&stack, val1, val2, ||, int64_t); break;
            case ADD: eval_binary_op(&stack, val1, val2, +,  int64_t);  break;
            case SUB: eval_binary_op(&stack, val1, val2, -,  int64_t);  break;
            case MUL: eval_binary_op(&stack, val1, val2, *,  int64_t);  break;
            case DIV: eval_binary_op(&stack, val2, val1, /,  int64_t);  break;
            case MOD: eval_binary_op(&stack, val2, val1, %,  int64_t);  break;
            case EQ:  eval_binary_op(&stack, val1, val2, ==, int64_t); break;
            case NEQ: eval_binary_op(&stack, val1, val2, !=, int64_t); break;
            case LTE: eval_binary_op(&stack, val1, val2, <=, int64_t); break;
            case GTE: eval_binary_op(&stack, val1, val2, >=, int64_t); break;
            case LT:  eval_binary_op(&stack, val1, val2, <,  int64_t);  break;
            case GT:  eval_binary_op(&stack, val1, val2, >,  int64_t);  break;
            case UNARY_PLUS: val1 = (int64_t) pop(&stack);
                push(&stack, (uint64_t) val1); break;
            case UNARY_MINUS: val1 = (int64_t) pop(&stack);
                push(&stack, (uint64_t) (-1 * val1)); break;
            case SET_LOCAL:
                def(&stack, &sfs);
                // print_frames(&sfs);
                break;
            case GET_LOCAL:
                ip++;
                symbol = instructions[ip];
                lookup_local(&sfs, symbol, &val1);
                push(&stack, (uint64_t) val1);
                // if (lookup_local(&locals, symbol, &val1)) {
                //     push(&stack, (uint64_t) val1);
                // } else {
                //     printf("variable '%s' is undefined\n", lookup_symbol(symbol));
                //     goto exit_loop;
                // }
                break;
            case JE:
                assert("JE note implemented\n" && false);
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
            case PUSH_SCOPE:
                stack_frame_enter(&sfs);
                break;
            case POP_SCOPE:
                stack_frame_exit(&sfs);
                break;
            default:
                printf("unknown instruction encountered: '%" PRIu64 "'", instructions[ip]);
                break;
        }
        ip++;
        i++;
        // print_stack(&stack);
        // if (i > 100) {
        //     printf("infinite loop detected, ending execution\n");
        //     break;
        // }
    }
exit_loop:
    print_stack(&stack);
    print_frames(&sfs);
    print_heap(&heap);
    return ret;
}

#undef eval_binary_op
