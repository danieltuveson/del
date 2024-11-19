#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"
#include "vector.h"

typedef uint64_t HeapPointer;

static inline void set_count(HeapPointer *ptr, size_t count)
{
    assert(count <= UINT16_MAX);
    *ptr = *ptr | (count << COUNT_OFFSET);
}

static inline size_t get_count(HeapPointer ptr)
{
    return (ptr & COUNT_MASK) >> COUNT_OFFSET;
}

static inline void set_metadata(HeapPointer *ptr, size_t metadata)
{
    assert(metadata <= UINT16_MAX);
    *ptr = *ptr | (metadata << METADATA_OFFSET);
}

static inline size_t get_metadata(HeapPointer ptr)
{
    return (ptr & METADATA_MASK) >> METADATA_OFFSET;
}

static inline void gc_mark(HeapPointer *ptr)
{
    *ptr = *ptr | GC_MARK_MASK;
}

static inline void gc_unmark(HeapPointer *ptr)
{
    *ptr = *ptr & ~GC_MARK_MASK;
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

static inline void push_floating(struct Stack *stack, double floating)
{
    stack->values[stack->offset++].floating = floating;
}

static inline void push_offset(struct Stack *stack, size_t offset)
{
    stack->values[stack->offset++].offset = offset;
}

#define push_chars(stack, characters) \
    memcpy(stack->values[stack->offset++].chars, characters, 8)

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
}

// Exchanges values on the stack at index1 and index2
static inline void switch_op(struct Stack *stack)
{
    DelValue index1, index2, temp;
    index1 = pop(stack);
    index2 = pop(stack);
    temp = stack->values[index1.offset];
    stack->values[index1.offset] = stack->values[index2.offset];
    stack->values[index2.offset] = temp;
}

#define eval_binary_op(stack_ptr, v1, v2, op) do { \
    v1 = pop(stack_ptr); \
    v2 = pop(stack_ptr); \
    push_integer(stack_ptr, (v2.integer op v1.integer)); \
} while (0)

#define eval_binary_op_f(stack_ptr, v1, v2, op) do { \
    v1 = pop(stack_ptr); \
    v2 = pop(stack_ptr); \
    push_floating(stack_ptr, (v2.floating op v1.floating)); \
    errno = 0; \
} while (0)

/* Pops values from the stack and pushes them onto the heap */
static inline bool push_heap(struct Heap *heap, struct Stack *stack)
{
    size_t count = pop(stack).offset;
    size_t metadata = pop(stack).offset;
    size_t ptr = heap->vector->length;
    set_count(&ptr, count);
    set_metadata(&ptr, metadata);
    size_t new_usage = heap->vector->length + count;
    if (new_usage > heap->vector->max_capacity) {
        printf("Fatal runtime error: out of memory\n");
        printf("Requested %lu bytes but VM only has a capacity of %lu bytes\n",
                IN_BYTES(new_usage), 
                IN_BYTES(heap->vector->max_capacity));
        return false;
    }
    // heap->values[heap->vector->length++] = count;
    // printf("count: %" PRIu64 "\n", count);
    // printf("location: %" PRIu64 "\n", location);
    for (size_t i = 0; i < count; i++) {
        DelValue value = pop(stack);
        // char str[9] = {0};
        // uint64_as_string(value, str, 0);
        // printf("value: %" PRIu64 "\n", value);
        vector_append(&(heap->vector), value);
    }
    // Store count in bits before location
    push_offset(stack, ptr);
    heap->objcount++;
#if DEBUG
    print_heap(heap);
#endif
    return true;
}

/* Get value from the heap and push it onto the stack */
static inline bool get_heap(struct Heap *heap, struct Stack *stack)
{
    size_t index = 2 * pop(stack).offset;
    size_t ptr = pop(stack).offset;
    size_t location = get_location(ptr);
    if (ptr == 0) {
        return false;
    }
    push_offset(stack, heap->vector->values[location + index].offset);
    return true;
}

static inline void set_heap(struct Heap *heap, struct Stack *stack)
{
    size_t index = 2 * pop(stack).offset;
    size_t ptr = pop(stack).offset;
    DelValue value = pop(stack);
    size_t location = get_location(ptr);
    heap->vector->values[location + index] = value;
}

static inline void stack_frame_enter(struct StackFrames *sfs)
{
    // vector_append(sfs->frame_offsets, sfs->frames->length);
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

static inline DelValue get_local(struct StackFrames *sfs, size_t scope_offset)
{
    size_t sf_offset = stack_frame_offset(sfs);
    return sfs->values[sf_offset + scope_offset];
}

static inline void set_local(struct Stack *stack, struct StackFrames *sfs, size_t scope_offset)
{
    DelValue val = pop(stack);
    size_t sf_offset = stack_frame_offset(sfs);
    sfs->values[sf_offset + scope_offset] = val;
#if DEBUG
    print_frames(sfs);
#endif
}

static void print_string(struct Stack *stack, struct Heap *heap)
{
    size_t ptr = pop(stack).offset;
    size_t location = get_location(ptr);
    size_t count = get_count(ptr);
    for (int i = count - 1; i >= 0; i--) {
        printf("%.*s", 8, heap->vector->values[location + i].chars);
    }
}

static void print(struct Stack *stack, struct Heap *heap)
{
    size_t ptr, location, count;
    DelValue dval, dtype;
    dtype = pop(stack);
    Type t = dtype.type;
    switch (t) {
        case TYPE_NULL:
            dval = pop(stack);
            printf("null");
            break;
        case TYPE_INT:
            dval = pop(stack);
            printf("%" PRIi64 "", dval.integer);
            break;
        case TYPE_FLOAT:
            dval = pop(stack);
            printf("%f", dval.floating);
            break;
        case TYPE_BOOL:
            dval = pop(stack);
            if (dval.integer == 0) {
                printf("false");
            } else {
                printf("true");
            }
            break;
        case TYPE_STRING:
            print_string(stack, heap);
            break;
        default:
            ptr = pop(stack).offset;
            location = get_location(ptr);
            count = get_count(ptr) / 2;
            if (location == 0) {
                printf("null");
            } else {
                printf("<%lu>, of size %lu", location, count);
            }
    }
}

static inline bool read(struct Stack *stack, struct Heap *heap)
{
    char packed[8] = {0};
    uint64_t i = 0;
    size_t offset = 0;
    while (true) {
        int c = getchar();
        if (c == EOF || c == '\n') {
            if (c == EOF) clearerr(stdin);
            break;
        }
        packed[offset] = (char) c;
        if (offset == 7) {
            push_chars(stack, packed);
            memset(packed, 0, 8);
        }
        i++;
        offset = i % 8;
    }
    // Push the remainder, if we haven't already
    if (offset != 0) {
        push_chars(stack, packed);
    }
    size_t metadata = i / 8 + (offset == 0 ? 0 : 1);
    push_offset(stack, offset);
    push_offset(stack, metadata);
    return push_heap(heap, stack);
}

// static inline void concat(struct Stack *stack, struct Heap *heap)
// {
//     size_t ptr = pop(stack).offset;
//     size_t location = get_location(ptr);
//     size_t count = get_count(ptr);
// 
//     char packed[8] = {0};
//     uint64_t i = 0;
//     size_t offset = 0;
//     int c = getchar();
//     while (c != EOF && c != '\n') {
//         packed[offset] = (char) c;
//         if (offset == 7) {
//             push_chars(stack, packed);
//             memset(packed, 0, 8);
//         }
//         i++;
//         offset = i % 8;
//         c = getchar();
//     }
//     // Push the remainder, if we haven't already
//     if (offset != 0) {
//         push_chars(stack, packed);
//     }
//     size_t metadata = i / 8 + (offset == 0 ? 0 : 1);
//     push_offset(stack, offset);
//     push_offset(stack, metadata);
//     return push_heap(heap, stack);
// }

// Assumes that vm is stack allocated / zeroed out
void vm_init(struct VirtualMachine *vm, DelValue *instructions)
{
    vm->heap.vector = vector_new(128, HEAP_MAX);
    vm->instructions = instructions;
}

void vm_free(struct VirtualMachine *vm)
{
    vector_free(vm->heap.vector);
}

#if DEBUG
#define emergency_break() do {\
    print_stack(&stack);\
    print_heap(&heap);\
    if (iterations > 200000) {\
        printf("infinite loop detected, ending execution\n");\
        status = VM_STATUS_ERROR;\
        goto exit_loop;\
    }\
} while(0)
#else
#define emergency_break()
#endif

#define pause_after(n) do {\
    if (iterations % n == 0) {\
        status = VM_STATUS_PAUSE;\
        goto exit_loop;\
    }\
} while(0)

#define on_break() do {\
    ip++;\
    iterations++;\
    /* For fun, lets exit whenever we reach 100 iterations */ \
    /* pause_after(100); */\
    emergency_break();\
} while(0)


uint64_t vm_execute(struct VirtualMachine *vm)
{
    // Define local variables for VM fields, for convenience (and maybe efficiency)
    enum VirtualMachineStatus status = vm->status;
    struct StackFrames sfs = vm->sfs;
    struct Stack stack = vm->stack;
    struct Heap heap = vm->heap;
    size_t ip = vm->ip;
    size_t scope_offset = vm->scope_offset;
    uint64_t ret = vm->ret;
    DelValue val1 = vm->val1;
    DelValue val2 = vm->val2;
    size_t iterations = vm->iterations;
    DelValue *instructions = vm->instructions;
#include "threading.h"
    while (1) {
        switch (instructions[ip].opcode) {
            vm_case(PUSH):
                ip++;
                push(&stack, instructions[ip]);
#if DEBUG
                print_stack(&stack);
                print_heap(&heap);
#endif
                vm_break;
            #define PUSH_N(n)\
            vm_case(PUSH_ ## n):\
                push_integer(&stack, n);\
                vm_break
            PUSH_N(0);
            PUSH_N(1);
            PUSH_N(2);
            PUSH_N(3);
            #undef PUSH_N
            vm_case(PUSH_HEAP):
                if (!push_heap(&heap, &stack)) {
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(SET_HEAP):
                set_heap(&heap, &stack);
                vm_break;
            /* Grotesque lump of binary operators. Boring! */
            vm_case(AND): eval_binary_op(&stack, val1, val2, &&); vm_break;
            vm_case(OR):  eval_binary_op(&stack, val1, val2, ||); vm_break;
            vm_case(ADD): eval_binary_op(&stack, val1, val2, +);  vm_break;
            vm_case(SUB): eval_binary_op(&stack, val1, val2, -);  vm_break;
            vm_case(MUL): eval_binary_op(&stack, val1, val2, *);  vm_break;
            vm_case(DIV): 
                val1 = pop(&stack);
                val2 = pop(&stack);
                if (val1.integer == 0) {
                    printf("Error: division by zero\n");
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                push_integer(&stack, (val2.integer / val1.integer));
                vm_break;
            vm_case(MOD):
                val1 = pop(&stack);
                val2 = pop(&stack);
                if (val1.integer == 0) {
                    printf("Error: division by zero\n");
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                push_integer(&stack, (val2.integer % val1.integer));
                vm_break;
            vm_case(EQ):  eval_binary_op(&stack, val1, val2, ==); vm_break;
            vm_case(NEQ): eval_binary_op(&stack, val1, val2, !=); vm_break;
            vm_case(LTE): eval_binary_op(&stack, val1, val2, <=); vm_break;
            vm_case(GTE): eval_binary_op(&stack, val1, val2, >=); vm_break;
            vm_case(LT):  eval_binary_op(&stack, val1, val2, <);  vm_break;
            vm_case(GT):  eval_binary_op(&stack, val1, val2, >);  vm_break;
            vm_case(UNARY_PLUS):
                val1 = pop(&stack);
                push_integer(&stack, val1.integer);
                vm_break;
            vm_case(UNARY_MINUS):
                val1 = pop(&stack);
                push_integer(&stack, (-1 * val1.integer));
                vm_break;
            vm_case(SET_LOCAL):
                ip++;
                scope_offset = instructions[ip].offset;
                set_local(&stack, &sfs, scope_offset);
                vm_break;
            #define SET_LOCAL_N(n)\
            vm_case(SET_LOCAL_ ## n):\
                set_local(&stack, &sfs, n);\
                vm_break
            SET_LOCAL_N(0);
            SET_LOCAL_N(1);
            SET_LOCAL_N(2);
            SET_LOCAL_N(3);
            #undef SET_LOCAL_N
            vm_case(DEFINE):
                sfs.index++;
                vm_break;
            vm_case(GET_LOCAL):
                ip++;
                scope_offset = instructions[ip].offset;
                val1 = get_local(&sfs, scope_offset);
                push(&stack, val1);
                vm_break;
            #define GET_LOCAL_N(n)\
            vm_case(GET_LOCAL_ ## n):\
                val1 = get_local(&sfs, n);\
                push(&stack, val1);\
                vm_break
            GET_LOCAL_N(0);
            GET_LOCAL_N(1);
            GET_LOCAL_N(2);
            GET_LOCAL_N(3);
            #undef GET_LOCAL_N
            vm_case(JE):
                assert("JE not implemented\n" && false);
                vm_break;
            vm_case(JNE):
                val1 = pop(&stack);
                if (!val1.integer) {
                    ip = pop(&stack).offset;
                    ip--; // reverse the effects of the ip++ in vm_break
                } else {
                    pop(&stack);
                }
                vm_break;
            vm_case(JMP):
                ip = pop(&stack).offset;
                ip--; // reverse the effects of the ip++ in vm_break
                vm_break;
            vm_case(POP):
                ret = pop(&stack).integer;
                vm_break;
            vm_case(GET_HEAP):
                if (!get_heap(&heap, &stack)) {
                    printf("Error: null pointer exception\n");
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(EXIT):
                status = VM_STATUS_COMPLETED;
                goto exit_loop;
            vm_case(CALL):
                // symbol = (Symbol) pop(&stack);
                // if (strcmp(lookup_symbol(symbol), "print") == 0) {
                //     printf("%s\n", (char *) pop(&stack));
                // } else {
                assert("unknown function encountered\n" && 0);
                // }
                vm_break;
            vm_case(SWAP):
                swap(&stack);
                vm_break;
            vm_case(SWITCH):
                switch_op(&stack);
                vm_break;
            vm_case(PUSH_SCOPE):
                stack_frame_enter(&sfs);
                vm_break;
            vm_case(POP_SCOPE):
                stack_frame_exit(&sfs);
                vm_break;
            vm_case(READ):
                if (!read(&stack, &heap)) {
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(PRINT): {
                print(&stack, &heap);
                vm_break;
            }
            vm_case(FLOAT_ADD): eval_binary_op_f(&stack, val1, val2, +);  vm_break;
            vm_case(FLOAT_SUB): eval_binary_op_f(&stack, val1, val2, -);  vm_break;
            vm_case(FLOAT_MUL): eval_binary_op_f(&stack, val1, val2, *);  vm_break;
            vm_case(FLOAT_DIV): eval_binary_op_f(&stack, val2, val1, /);  vm_break;
            vm_case(FLOAT_EQ):  eval_binary_op_f(&stack, val1, val2, ==); vm_break;
            vm_case(FLOAT_NEQ): eval_binary_op_f(&stack, val1, val2, !=); vm_break;
            vm_case(FLOAT_LTE): eval_binary_op_f(&stack, val1, val2, <=); vm_break;
            vm_case(FLOAT_GTE): eval_binary_op_f(&stack, val1, val2, >=); vm_break;
            vm_case(FLOAT_LT):  eval_binary_op_f(&stack, val1, val2, <);  vm_break;
            vm_case(FLOAT_GT):  eval_binary_op_f(&stack, val1, val2, >);  vm_break;
            vm_case(FLOAT_UNARY_PLUS):
                val1 = pop(&stack);
                push_floating(&stack, val1.floating);
                vm_break;
            vm_case(FLOAT_UNARY_MINUS):
                val1 = pop(&stack);
                push_floating(&stack, (-1 * val1.floating));
                vm_break;
            vm_case(PUSH_STRING):
            vm_case(GET_CHAR):
            vm_case(SET_CHAR):
            default:
                printf("unknown instruction encountered: '%" PRIu64 "'", instructions[ip].offset);
                status = VM_STATUS_ERROR;
                goto exit_loop;
        }
        // Note: any statements to be executed before looping back to the top should be done
        // in the on_break macro. When threaded code is enabled, statements below this line
        // will not execute
    }
exit_loop:
#if DEBUG
    print_stack(&stack);
    print_frames(&sfs);
    print_heap(&heap);
#endif
    // Update state of VM before exiting
    vm->status = status;
    vm->sfs = sfs;
    vm->stack = stack;
    vm->heap = heap;
    vm->ip = ip;
    vm->scope_offset = scope_offset;
    vm->ret = ret;
    vm->val1 = val1;
    vm->val2 = val2;
    vm->iterations = iterations;
    vm->instructions = instructions;
    return ret;
}

#undef eval_binary_op
