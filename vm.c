#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"
#include "vector.h"

typedef uint64_t HeapPointer;

static inline void set_count(HeapPointer *ptr, size_t count)
{
    assert(count <= UINT16_MAX * 3 / 2);
    *ptr = *ptr | (count << COUNT_OFFSET);
}

static inline size_t get_count(HeapPointer ptr)
{
    return (ptr & COUNT_MASK) >> COUNT_OFFSET;
}

static inline void set_metadata(HeapPointer *ptr, size_t metadata)
{
    assert(metadata <= UINT8_MAX);
    *ptr = *ptr | (metadata << METADATA_OFFSET);
}

static inline size_t get_metadata(HeapPointer ptr)
{
    return (ptr & METADATA_MASK) >> METADATA_OFFSET;
}

static inline size_t get_location(HeapPointer ptr)
{
    return ptr & LOCATION_MASK;
}

static inline void gc_mark(HeapPointer *ptr)
{
    *ptr = *ptr | GC_MARK_MASK;
}

static inline void gc_unmark(HeapPointer *ptr)
{
    *ptr = *ptr & ~GC_MARK_MASK;
}

static inline bool gc_is_marked(HeapPointer ptr)
{
    return ((ptr & GC_MARK_MASK) >> GC_MARK_OFFSET);
}

// Our conservative GC looks for potential pointers in the stack / stack frames.
// If we make pointers be numbers less likely to appear in a normal program,
// we lessen the likelyhood of leaving memory around that should have beeen freed

// In binary this is 010101...
// const uint64_t zero_one_zero_etc = UINT64_C(6148914691236517205);
// 
// static inline HeapPointer encode_pointer(HeapPointer ptr)
// {
//     return ~(ptr ^ zero_one_zero_etc);
// }
// 
// static inline HeapPointer decode_pointer(HeapPointer ptr)
// {
//     return (~ptr) ^ zero_one_zero_etc;
// }

/*
struct StackFrames {
    size_t index;
    DelValue *values;
    size_t frame_offsets_index;
    size_t *frame_offsets;
};

// The stack stores almost all data used by the VM 
struct Stack {
    size_t offset;
    DelValue *values;
};

// A value on the heap is just a slice of bytes
struct Heap {
    struct Vector *vector;
};
*/

// // Checks if pointer follows correct struct of heap pointer.
// // It still *might* not be a valid heap pointer.
// static HeapPointer is_valid_pointer(HeapPointer maybe_ptr, struct Heap *heap, bool is_internal)
// {
//     HeapPointer ptr = decode_pointer(maybe_ptr);
//     size_t loc = get_location(ptr);
//     size_t count = get_count(ptr);
//     if (count + loc >= heap->vector->length) {
//         printf("invalid - out of bounds\n");
//         return 0;
//     }
//     if (!is_internal) {
//         printf("invalid - already flagged\n");
//         // Shouldn't be marked yet
//         if (gc_is_marked(ptr)) {
//             return 0;
//         }
//     }
//     return 0;
// }

// static bool gc_mark_children(HeapPointer ptr, struct Heap *heap)
// {
//     (void)ptr;(void)heap;
//     return false;
// }
// 
// static void gc_collect(struct Heap *heap, struct Stack *stack, struct StackFrames *sfs)
// {
//     (void)sfs;
//     struct Vector *new_heap = vector_new(HEAP_INIT, HEAP_MAX);
//     struct Vector *ptrs = vector_new(8, UINT64_MAX);
//     for (size_t i = 0; i < stack->offset; i++) {
//         HeapPointer ptr = is_valid_pointer(stack->values[i].offset, heap, false);
//         if (ptr) {
//             gc_mark_children(ptr, heap);
//         }
//     }
//     heap->gc_threshold = GC_GROWTH_FACTOR * new_heap->capacity;
//     heap->vector = new_heap;
//     vector_free(ptrs);
//     return;
// }

// 
// static void gc_sweep()
// {
// }

// NOTE: push does not check for overflow
// Any call of push that is not preceded by an equal or greater number of pops
// should check for overflow
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

static inline void dup(struct Stack *stack)
{
    DelValue v = pop(stack);
    push(stack, v);
    push(stack, v);
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
// TODO: Rewrite this + compiler so that push_heap allocates but doesn't set anything
static inline bool push_heap(struct Heap *heap, struct Stack *stack, struct StackFrames *sfs)
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
    } else if (heap->vector->length == heap->vector->capacity && heap->vector->length > 0) {
        // Pulled from vector.c logic
        // Only want to GC when we are on the verge of needing to grow array
        // gc_collect(heap, stack, sfs);
    }
    // vector_grow(&(heap->vector), count);
    for (size_t i = 0; i < count; i++) {
        DelValue value = pop(stack);
        vector_append(&(heap->vector), value);
    }
    // Store count in bits before location
    push_offset(stack, ptr);
#if DEBUG
    print_heap(heap);
#endif
    return true;
}

static inline bool push_array(struct Heap *heap, struct Stack *stack, struct StackFrames *sfs)
{
    int64_t dirty_length = pop(stack).integer;
    size_t obj_size = pop(stack).offset;
    if (dirty_length < 1) {
        printf("Fatal runtime error: index of array less than 1\n");
        return false;
    }
    size_t ptr = heap->vector->length;
    size_t length = (size_t) dirty_length;
    size_t count = obj_size * length;
    set_count(&ptr, count);
    size_t new_usage = heap->vector->length + count;
    if (new_usage > heap->vector->max_capacity) {
        printf("Fatal runtime error: out of memory\n");
        printf("Requested %lu bytes but VM only has a capacity of %lu bytes\n",
                IN_BYTES(new_usage), 
                IN_BYTES(heap->vector->max_capacity));
        return false;
    } else if (heap->vector->length == heap->vector->capacity && heap->vector->length > 0) {
        // Pulled from vector.c logic
        // Only want to GC when we are on the verge of needing to grow array
        // gc_collect(heap, stack, sfs);
    }
    vector_grow(&(heap->vector), count);
    // Store count in bits before location
    push_offset(stack, ptr);
#if DEBUG
    print_heap(heap);
#endif
    return true;
}

/* Get value from the heap and push it onto the stack */
static inline bool get_heap(struct Heap *heap, struct Stack *stack)
{
    size_t index = 1 + 5 * pop(stack).offset / 4;
    size_t ptr = pop(stack).offset;
    size_t location = get_location(ptr);
    if (ptr == 0) {
        return false;
    }
    push(stack, heap->vector->values[location + index]);
    return true;
}

static inline void set_heap(struct Heap *heap, struct Stack *stack)
{
    size_t index = 1 + 5 * pop(stack).offset / 4;
    size_t ptr = pop(stack).offset;
    DelValue value = pop(stack);
    size_t location = get_location(ptr);
    heap->vector->values[location + index] = value;
}

static inline bool get_array(struct Heap *heap, struct Stack *stack)
{
    int64_t index = pop(stack).integer;
    size_t ptr = pop(stack).offset;
    size_t location = get_location(ptr);
    size_t count = get_count(ptr);
    if (index < 0 || index >= (int64_t)count) {
        printf("Error: array index out of bounds exception\n");
        return false;
    }
    push(stack, heap->vector->values[location + index]);
    return true;
}

static inline bool set_array(struct Heap *heap, struct Stack *stack)
{
    int64_t index = pop(stack).integer;
    size_t ptr = pop(stack).offset;
    size_t location = get_location(ptr);
    size_t count = get_count(ptr);
    if (index < 0 || index >= (int64_t)count) {
        printf("Error: array index out of bounds exception\n");
        return false;
    }
    DelValue value = pop(stack);
    heap->vector->values[location + index] = value;
    return true;
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

static inline DelValue get_local(struct StackFrames *sfs, size_t scope_offset)
{
    size_t sf_offset = stack_frame_offset(sfs);
    return sfs->values[sf_offset + scope_offset];
}

static inline void set_local(struct Stack *stack, struct StackFrames *sfs, size_t scope_offset)
{
    // size_t type = pop(stack).offset;
    DelValue val = pop(stack);
    size_t sf_offset = stack_frame_offset(sfs);
    // sfs->values[sf_offset + scope_offset].offset = type;
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

// TODO: Check that stack does not overflow when pushing
static inline bool read(struct Stack *stack, struct Heap *heap, struct StackFrames *sfs)
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
    return push_heap(heap, stack, sfs);
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
    vm->stack.values = calloc(STACK_MAX, sizeof(*(vm->stack.values)));
    vm->sfs.values = calloc(STACK_MAX, sizeof(*(vm->sfs.values)));
    vm->sfs.frame_offsets = calloc(STACK_MAX, sizeof(*(vm->sfs.frame_offsets)));
    vm->heap.vector = vector_new(HEAP_INIT, HEAP_MAX);
    vm->heap.gc_threshold = GC_GROWTH_FACTOR * vm->heap.vector->capacity;
    vm->instructions = instructions;
}

void vm_free(struct VirtualMachine *vm)
{
    free(vm->stack.values);
    free(vm->sfs.values);
    free(vm->sfs.frame_offsets);
    vector_free(vm->heap.vector);
}

#if DEBUG
#define emergency_break() do {\
    print_stack(&stack);\
    print_heap(&heap);\
    /* if (iterations > 200000) {*/\
    if (iterations > 200) {\
        printf("infinite loop detected, ending execution\n");\
        status = VM_STATUS_ERROR;\
        goto exit_loop;\
    }\
} while(0)
#else
#define emergency_break()
#endif

#define pause_after(n) do {\
    if (unexpected(iterations % n == 0)) {\
        status = VM_STATUS_PAUSE;\
        goto exit_loop;\
    }\
} while(0)

#define on_break() do {\
    ip++;\
    iterations++;\
    /* For fun, lets exit whenever we reach 100 iterations */ \
    /* pause_after(UINT64_MAX);*/\
    emergency_break();\
} while(0)

// Bounds check on pushes to stack
#define check_push() do {\
    if (unexpected(stack.offset >= STACK_MAX - 1)) { \
        printf("Error: stack overflow (calculation too large)\n");\
        status = VM_STATUS_ERROR;\
        goto exit_loop;\
    }\
} while(0)

uint64_t vm_execute(struct VirtualMachine *vm)
{
    // Define local variables for VM fields, for convenience (and maybe efficiency)
    enum VirtualMachineStatus status = vm->status;
    struct StackFrames sfs = vm->sfs;
    struct Stack stack = vm->stack;
    struct Heap heap = vm->heap;
    size_t ip = vm->ip;
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
                check_push();
                push(&stack, instructions[ip]);
#if DEBUG
                print_stack(&stack);
                print_heap(&heap);
#endif
                vm_break;
            #define PUSH_N(n)\
            vm_case(PUSH_ ## n):\
                check_push();\
                push_integer(&stack, n);\
                vm_break
            PUSH_N(0);
            PUSH_N(1);
            PUSH_N(2);
            PUSH_N(3);
            #undef PUSH_N
            vm_case(PUSH_HEAP):
                if (!push_heap(&heap, &stack, &sfs)) {
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(PUSH_ARRAY):
                if (!push_array(&heap, &stack, &sfs)) {
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(LEN_ARRAY):
                TODO();
                vm_break;
            vm_case(GET_HEAP):
                if (!get_heap(&heap, &stack)) {
                    printf("Error: null pointer exception\n");
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(SET_HEAP):
                set_heap(&heap, &stack);
                vm_break;
            vm_case(GET_ARRAY):
                if (!get_array(&heap, &stack)) {
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(SET_ARRAY):
                if (!set_array(&heap, &stack)) {
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(DUP):
                dup(&stack);
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
                set_local(&stack, &sfs, instructions[ip].offset);
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
                // TODO: Figure out how to make this compile-time only
                sfs.index++;
                vm_break;
            vm_case(GET_LOCAL):
                ip++;
                val1 = get_local(&sfs, instructions[ip].offset);
                check_push();
                push(&stack, val1);
                vm_break;
            #define GET_LOCAL_N(n)\
            vm_case(GET_LOCAL_ ## n):\
                val1 = get_local(&sfs, n);\
                check_push();\
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
                if (sfs.index >= STACK_MAX - 1 || sfs.frame_offsets_index >= STACK_MAX - 1) {
                    printf("Error: stack overflow\n");
                    status = VM_STATUS_ERROR;
                    goto exit_loop;
                }
                stack_frame_enter(&sfs);
                vm_break;
            vm_case(POP_SCOPE):
                stack_frame_exit(&sfs);
                vm_break;
            vm_case(READ):
                if (!read(&stack, &heap, &sfs)) {
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
                printf("unknown instruction encountered: '%" PRIu64 "'",
                        instructions[ip].offset);
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
    vm->ret = ret;
    vm->val1 = val1;
    vm->val2 = val2;
    vm->iterations = iterations;
    vm->instructions = instructions;
    return ret;
}

#undef eval_binary_op
