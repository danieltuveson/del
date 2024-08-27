#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"

typedef size_t HeapPointer;

static inline void set_count(HeapPointer *ptr, size_t count)
{
    assert(count <= UINT16_MAX);
    *ptr = *ptr | (count << COUNT_OFFSET);
}

static inline size_t get_count(HeapPointer ptr)
{
    return (ptr & COUNT_MASK) >> COUNT_OFFSET;
}

static inline size_t get_location(HeapPointer ptr)
{
    return ptr & LOCATION_MASK;
}

// int64_t integer;
// size_t offset;
// double floating;
// char character;
// enum Code opcode;
static inline void push(struct Stack *stack, DelValue val)
{
    stack->values[stack->offset++] = val;
}

static inline void push_integer(struct Stack *stack, int64_t integer)
{
    stack->values[stack->offset++].integer = integer;
}

static inline void push_offset(struct Stack *stack, size_t offset)
{
    stack->values[stack->offset++].offset = offset;
}

static inline DelValue pop(struct Stack *stack)
{
    return stack->values[--stack->offset];
}

static inline void swap(struct Stack *stack)
{
    DelValue val1, val2;
    val1 = pop(stack);
    val2 = pop(stack);
    push(stack, val1);
    push(stack, val2);
//     uint64_t swp = stack->values[stack->offset];
//     stack->values[stack->offset] = stack->values[stack->offset-1];
//     stack->values[stack->offset-1] = swp;
}

#define eval_binary_op(stack_ptr, v1, v2, op) \
    do { \
        v1 = pop(stack_ptr); \
        v2 = pop(stack_ptr); \
        push_integer(stack_ptr, (v2.integer op v1.integer)); } while (0)

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
    size_t count = pop(stack).offset;
    size_t ptr = heap->offset;
    set_count(&ptr, count);
    // heap->values[heap->offset++] = count;
    // printf("count: %" PRIu64 "\n", count);
    // printf("location: %" PRIu64 "\n", location);
    for (size_t i = 0; i < count; i++) {
        DelValue value = pop(stack);
        // char str[9] = {0};
        // uint64_as_string(value, str, 0);
        // printf("value: %" PRIu64 "\n", value);
        heap->values[heap->offset++] = value;
    }
    // Store count in bits before location
    push_offset(stack, ptr);
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
    size_t index = pop(stack).offset;
    size_t ptr = pop(stack).offset;
    size_t location = get_location(ptr);
    push_offset(stack, heap->values[location + index].offset);
}

static inline void set_heap(struct Heap *heap, struct Stack *stack)
{
    printf("setting heap\n");
    size_t index = pop(stack).offset;
    size_t ptr = pop(stack).offset;
    DelValue value = pop(stack);
    size_t location = get_location(ptr);
    heap->values[location + index] = value;
    printf("value: %" PRIi64 ", ptr: %" PRIu64 ", index: %" PRIu64 ", location: %" PRIu64 "\n", 
           value.integer, ptr, index, location);
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

static DelValue get_local(struct StackFrames *sfs, size_t scope_offset)
{
    size_t sf_offset = stack_frame_offset(sfs);
    return sfs->values[sf_offset + scope_offset];
}

static void set_local(struct Stack *stack, struct StackFrames *sfs, size_t scope_offset)
{
    DelValue val = pop(stack);
    size_t sf_offset = stack_frame_offset(sfs);
    sfs->values[sf_offset + scope_offset] = val;
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

int vm_execute(DelValue *instructions)
{
#if BENCHMARK
    int popcount = 0;
#endif
    struct StackFrames sfs = {0, {0}, {0}, 0, {0}};
    struct Stack stack = {0, {0}};
    struct Heap heap = {0, 0, {0}};
    size_t ip = 0;
    size_t scope_offset = 0;
    uint64_t ret = 0;
    DelValue val1 = { .integer = 0 };
    DelValue val2 = { .integer = 0 };
    size_t iterations = 0;
    while (1) {
        switch (instructions[ip].opcode) {
            case PUSH:
                ip++;
                push(&stack, instructions[ip]);
                break;
            #define PUSH_N(n)\
            case PUSH_ ## n:\
                push_integer(&stack, n);\
                break
            PUSH_N(0);
            PUSH_N(1);
            PUSH_N(2);
            PUSH_N(3);
            #undef PUSH_N
            case PUSH_HEAP:
                push_heap(&heap, &stack);
#if DEBUG
                print_heap(&heap);
#endif
                break;
            case SET_HEAP:
                set_heap(&heap, &stack);
                break;
            /* Grotesque lump of binary operators. Boring! */
            case AND: eval_binary_op(&stack, val1, val2, &&); break;
            case OR:  eval_binary_op(&stack, val1, val2, ||); break;
            case ADD: eval_binary_op(&stack, val1, val2, +);  break;
            case SUB: eval_binary_op(&stack, val1, val2, -);  break;
            case MUL: eval_binary_op(&stack, val1, val2, *);  break;
            case DIV: eval_binary_op(&stack, val2, val1, /);  break;
            case MOD: eval_binary_op(&stack, val2, val1, %);  break;
            case EQ:  eval_binary_op(&stack, val1, val2, ==); break;
            case NEQ: eval_binary_op(&stack, val1, val2, !=); break;
            case LTE: eval_binary_op(&stack, val1, val2, <=); break;
            case GTE: eval_binary_op(&stack, val1, val2, >=); break;
            case LT:  eval_binary_op(&stack, val1, val2, <);  break;
            case GT:  eval_binary_op(&stack, val1, val2, >);  break;
            case UNARY_PLUS:
                val1 = pop(&stack);
                push_integer(&stack, val1.integer);
                break;
            case UNARY_MINUS:
                val1 = pop(&stack);
                push_integer(&stack, (-1 * val1.integer));
                break;
            case SET_LOCAL:
                ip++;
                scope_offset = instructions[ip].offset;
                set_local(&stack, &sfs, scope_offset);
#if DEBUG
                print_frames(&sfs);
#endif
                break;
#if DEBUG
            #define SET_LOCAL_N(n)\
            case SET_LOCAL_ ## n:\
                set_local(&stack, &sfs, n);\
                print_frames(&sfs);\
                break
#else
            #define SET_LOCAL_N(n)\
            case SET_LOCAL_ ## n:\
                set_local(&stack, &sfs, n);\
                break
#endif
            SET_LOCAL_N(0);
            SET_LOCAL_N(1);
            SET_LOCAL_N(2);
            SET_LOCAL_N(3);
            #undef SET_LOCAL_N
            case DEFINE:
                sfs.index++;
                break;
            case GET_LOCAL:
                ip++;
                scope_offset = instructions[ip].offset;
                val1 = get_local(&sfs, scope_offset);
                push(&stack, val1);
                break;
            #define GET_LOCAL_N(n)\
            case GET_LOCAL_ ## n:\
                val1 = get_local(&sfs, n);\
                push(&stack, val1);\
                break
            GET_LOCAL_N(0);
            GET_LOCAL_N(1);
            GET_LOCAL_N(2);
            GET_LOCAL_N(3);
            #undef GET_LOCAL_N
            case JE:
                assert("JE note implemented\n" && false);
                break;
            case JNE:
                val1 = pop(&stack);
                if (!val1.integer) {
                    ip = pop(&stack).offset;
                    ip--; // reverse the effects of the ip++ below
                } else {
                    pop(&stack);
                }
                break;
            case JMP:
                ip = pop(&stack).offset;
                ip--; // reverse the effects of the ip++ below
                break;
            case POP:
                ret = pop(&stack).integer;
                break;
            case GET_HEAP:
                get_heap(&heap, &stack);
                break;
            case EXIT:
                goto exit_loop;
            case CALL:
                // symbol = (Symbol) pop(&stack);
                // if (strcmp(lookup_symbol(symbol), "print") == 0) {
                //     printf("%s\n", (char *) pop(&stack));
                // } else {
                assert("unknown function encountered\n" && 0);
                // }
                break;
            case SWAP:
                swap(&stack);
                break;
            case PUSH_SCOPE:
                stack_frame_enter(&sfs);
                break;
            case POP_SCOPE:
#if BENCHMARK
                // print value for fibonacci benchmark
                if (popcount == 1000000) {
                    printf("x: %li\n", get_local(&sfs, 1).integer);
                }
#endif
                stack_frame_exit(&sfs);
#if BENCHMARK
                popcount++;
#endif
                break;
            case PRINT:
                break;
            default:
                printf("unknown instruction encountered: '%" PRIu64 "'", instructions[ip].offset);
                break;
        }
        ip++;
        iterations++;
#if DEBUG
        print_stack(&stack);
        // print_heap(&heap);
        if (iterations > 200000) {
            printf("infinite loop detected, ending execution\n");
            break;
        }
#endif
    }
exit_loop:
    print_stack(&stack);
    print_frames(&sfs);
    print_heap(&heap);
    return ret;
}

#undef eval_binary_op
