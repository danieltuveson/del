#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"
#include "vector.h"
#include "heap_ptr.h"
#include "gc.h"
#include "ffi.h"
#include "del.h"

// Walk the list of values that are alive, marking and putting into PointerRemaps
//
// old_loc has old location, new_loc = previous new_loc offset + count of old_loc pointer
//
// Basically we want to simulate adding them to the heap before we do it, we can update all of
// the pointers when we actually start adding them to the new heap
//
// unmark once we've moved them into new heap

static void print_object(struct Heap *heap, size_t location, size_t count, char **string_pool,
        FILE *fout);

// static void print_ptr(struct Heap *heap, HeapPointer ptr, char **string_pool)
// {
//     size_t location = get_location(ptr);
//     size_t count = get_count(ptr);
//     print_object(heap, location, count, string_pool);
//     printf("\n");
// }

#if 0
// #if DEBUG_RUNTIME
// TODO: make this not recursive since that could easily exhaust C stack if we have a large
// recursive datastructure like a linkedlist 
static void gc_mark_children(struct GarbageCollector *gc, HeapPointer ptr, struct Heap *heap,
        char **string_pool)
// #else
// static void gc_mark_children(struct GarbageCollector *gc, HeapPointer ptr, struct Heap *heap)
// #endif
{
    if (ptr == 0 || gc_is_marked(ptr)) {
        return;
    }
    gc_remap(gc, ptr);
    gc_mark(&ptr);
    size_t location = get_location(ptr);
    size_t count = get_count(ptr);
    // Loop through elements in heap object, mark inner objects
    if (is_array_ptr(ptr)) {
        if (is_array_of_objects(ptr)) {
            printf("marking child array: \n");
            for (size_t i = location; i < count + location; i++) {
                DelValue value = vector_get(heap->vector, i);
                gc_mark_children(gc, value.offset, heap, string_pool);
            }
        }
    } else {
        printf("marking child object: \n");
        print_object(heap, location, count, string_pool);
        printf("\n");
        uint16_t types[4] = {0};
        uint16_t type_index = 0;
        for (size_t i = 0; i < count; i++) {
            DelValue value = vector_get(heap->vector, i + location);
            if (i % 5 == 0) {
                memcpy(types, value.types, 8);
                type_index = 0;
            } else {
                Type type = types[type_index];
                if (is_object(type)) {
                    printf("recursing...\n");
                    gc_mark_children(gc, value.offset, heap, string_pool);
                }
                type_index++;
            }
        }
    }
}

// Copy all marked objects into new heap and update pointers to new locations
static void gc_move(struct GarbageCollector *gc, struct Stack *stack, struct StackFrames *sfs,
        struct Heap *heap, char **string_pool)
{
    printf("stack:\n");
    for (size_t i = 0; i < stack->offset; i++) {
        gc_remap_ptr(gc, &stack->values[i].offset);
        // gc_remap_children(&gc, ptr, heap);
        // gc_mark_children(&gc, ptr, heap, string_pool);
    }
    printf("locals:\n");
    for (size_t i = 0; i < sfs->index; i++) {
        gc_remap_ptr(gc, &sfs->values[i].offset);
        // gc_mark_children(&gc, ptr, heap, string_pool);
    }
}

static void gc_collect(struct Heap *heap, struct Stack *stack, struct StackFrames *sfs,
        char **string_pool)
{
    struct Vector *new_heap = vector_new(HEAP_INIT, HEAP_MAX);
    struct GarbageCollector gc;
    gc_init(&gc);
    printf("stack:\n");
    for (size_t i = 0; i < stack->offset; i++) {
        HeapPointer ptr = stack->values[i].offset;
        gc_mark_children(&gc, ptr, heap, string_pool);
    }
    printf("locals:\n");
    for (size_t i = 0; i < sfs->index; i++) {
        HeapPointer ptr = sfs->values[i].offset;
        gc_mark_children(&gc, ptr, heap, string_pool);
    }
    printf("remap:\n");
    print_remap(&gc);
    // heap->gc_threshold = GC_GROWTH_FACTOR * new_heap->capacity;
    // heap->vector = new_heap;
    vector_free(new_heap); // this is just to make valgrind happy until I do stuff
    gc_free(&gc);
    printf("========================\n");
    return;
}
#endif

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

static inline void push_byte(struct Stack *stack, char byte)
{
    stack->values[stack->offset++].byte = byte;
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
#if DEBUG_RUNTIME
    assert(stack->offset > 0);
#endif
    return stack->values[--stack->offset];
}

#define dup(stack_ptr) do {\
    val1 = pop(stack_ptr);\
    push(stack_ptr, val1);\
    check_push(stack_ptr);\
    push(stack_ptr, val1);\
} while(0)

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
static inline bool push_heap(size_t count, size_t metadata, struct Heap *heap, struct Stack *stack,
        struct Stack *stack_obj, struct StackFrames *sfs_obj, char **string_pool, FILE *ferr)
{
    size_t ptr = heap->vector->length;
    if (!set_count(&ptr, count)) {
        fprintf(ferr, "Fatal runtime error: object requires %lu bytes which exceeds maximum size of %lu"
                " bytes\n", IN_BYTES(count), IN_BYTES(COUNT_MAX));
        return false;
    }
    set_metadata(&ptr, metadata);
    size_t new_usage = heap->vector->length + count;
    // printf("new usage: %lu\n", new_usage);
    if (new_usage > heap->vector->max_capacity) {
        fprintf(ferr, "Fatal runtime error: requested %lu bytes but VM only has a capacity of %lu bytes\n",
                IN_BYTES(new_usage), 
                IN_BYTES(heap->vector->max_capacity));
        return false;
    } else if (heap->vector->length == heap->vector->capacity && heap->vector->length > 0) {
        // Pulled from vector.c logic
        // Only want to GC when we are on the verge of needing to grow array
        // gc_collect(heap, stack, sfs);
    }
    // vector_grow(&(heap->vector), count);
    uint16_t types[4] = {0};
    uint16_t type_index = 0;
    DelValue value;
    for (size_t i = 0; i < count; i++) {
        if (i % 5 == 0) {
            value = pop(stack);
            memcpy(types, value.types, 8);
            type_index = 0;
        } else {
            Type type = types[type_index];
            value = is_object_or_null(type) ? pop(stack_obj) : pop(stack);
            type_index++;
        }
        vector_append(&(heap->vector), value);
    }
    push_offset(stack_obj, ptr);
    // if (new_usage > 20) {
    // gc_collect(heap, stack_obj, sfs_obj, string_pool);
    // }
// #if DEBUG_RUNTIME
//     print_heap(heap);
// #endif
    return true;
}

// static inline bool push_array(struct Heap *heap, struct Stack *stack)//, struct StackFrames *sfs)
static inline bool push_array(struct Heap *heap, struct Stack *stack, struct Stack *stack_obj,
        FILE *ferr)
{
    size_t array_type = pop(stack).offset;
    int64_t dirty_count = pop(stack).integer;
    if (dirty_count < 1) {
        fprintf(ferr, "Fatal runtime error: index of array less than 1\n");
        return false;
    }
    size_t ptr = heap->vector->length;
    size_t count = (size_t) dirty_count;
    if (!set_count(&ptr, count)) {
        fprintf(ferr, "Fatal runtime error: object requires %lu bytes which exceeds maximum size of %lu"
                " bytes\n", IN_BYTES(count), IN_BYTES(COUNT_MAX));
        return false;
    }
    size_t new_usage = heap->vector->length + count;
    if (new_usage > heap->vector->max_capacity) {
        fprintf(ferr, "Fatal runtime error: out of memory\n");
        fprintf(ferr, "Requested %lu bytes but VM only has a capacity of %lu bytes\n",
                IN_BYTES(new_usage), 
                IN_BYTES(heap->vector->max_capacity));
        return false;
    } else if (heap->vector->length == heap->vector->capacity && heap->vector->length > 0) {
        // Pulled from vector.c logic
        // Only want to GC when we are on the verge of needing to grow array
        // gc_collect(heap, stack, sfs);
    }
    vector_grow(&(heap->vector), count);
    // Store metadata / count in bits before location
    set_array_bit(&ptr);
    if (is_object(array_type)) set_array_obj_bit(&ptr);
    push_offset(stack_obj, ptr);
// #if DEBUG_RUNTIME
//     print_heap(heap);
// #endif
    return true;
}

/* Get value from the heap and push it onto the stack */
// Technically we now need GET_HEAP_OBJ if the element we're pushing back
// onto the stack is an object
static inline bool get_heap(struct Heap *heap, size_t index, size_t ptr, struct Stack *stack)
{
    size_t location = get_location(ptr);
    if (ptr == 0) {
        return false;
    }
    push(stack, vector_get(heap->vector, location + index));
    return true;
}

// Do we now need SET_HEAP_OBJ, similar to GET_HEAP_OBJ?
static inline void set_heap(struct Heap *heap, size_t index, size_t ptr, DelValue value)
{
    size_t location = get_location(ptr);
    vector_set(heap->vector, location + index, value);
}

static inline bool get_array(int64_t index, size_t ptr, struct Heap *heap, struct Stack *stack,
        FILE *ferr)
{
    size_t location = get_location(ptr);
    size_t count = get_count(ptr);
    if (ptr == 0) {
        fprintf(ferr, "Error: null pointer exception\n");
        return false;
    } else if (index < 0 || index >= (int64_t)count) {
        fprintf(ferr, "Error: array index out of bounds exception\n");
        return false;
    }
    push(stack, vector_get(heap->vector, location + index));
    return true;
}

static inline bool set_array(int64_t index, size_t ptr, struct Heap *heap, struct Stack *stack)
{
    size_t location = get_location(ptr);
    size_t count = get_count(ptr);
    if (index < 0 || index >= (int64_t)count) {
        return false;
    }
    DelValue value = pop(stack);
    vector_set(heap->vector, location + index, value);
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
    DelValue val = pop(stack);
    size_t sf_offset = stack_frame_offset(sfs);
    sfs->values[sf_offset + scope_offset] = val;
}

// static void print_string(struct Stack *stack, struct Heap *heap)
// {
//     size_t ptr = pop(stack).offset;
//     size_t location = get_location(ptr);
//     size_t count = get_count(ptr);
//     for (int i = count - 1; i >= 0; i--) {
//         printf("%.*s", 8, vector_get(heap->vector, location + i).chars);
//     }
// }

static void print_primitive(Type type, DelValue dval, char **string_pool, FILE *fout)
{
    switch (type) {
        case TYPE_NULL:
            fprintf(fout, "null");
            break;
        case TYPE_BYTE:
            fprintf(fout, "%c", dval.byte);
            break;
        case TYPE_INT:
            fprintf(fout, "%" PRIi64 "", dval.integer);
            break;
        case TYPE_FLOAT:
            fprintf(fout, "%f", dval.floating);
            break;
        case TYPE_BOOL:
            if (dval.integer == 0) {
                fprintf(fout, "false");
            } else {
                fprintf(fout, "true");
            }
            break;
        case TYPE_STRING:
            fprintf(fout, "%s", string_pool[dval.offset]);
            break;
        default:
            assert(false);
    }
}

static void print_addr(size_t location, FILE *fout)
{
    if (location == 0) {
        fprintf(fout, "null");
    } else {
        fprintf(fout, "<%lu>", location);
    }
}

static void pprint_primitive(Type type, DelValue dval, char **string_pool, FILE *fout)
{
    if (type == TYPE_STRING) {
        fprintf(fout, "\"");
    } else if (type == TYPE_BYTE) {
        fprintf(fout, "'");
    }
    print_primitive(type, dval, string_pool, fout);
    if (type == TYPE_STRING) {
        fprintf(fout, "\"");
    } else if (type == TYPE_BYTE) {
        fprintf(fout, "'");
    }
}

static void print_object(struct Heap *heap, size_t location, size_t count, char **string_pool,
        FILE *fout)
{
    if (count == 0 && location == 0) {
        fprintf(fout, "null");
        return;
    }
    uint16_t types[4] = {0};
    uint16_t type_index = 0;
    fprintf(fout, "{ ");
    for (size_t i = 0; i < count; i++) {
        DelValue value = vector_get(heap->vector, location + i);
        if (i % 5 == 0) {
            memcpy(types, value.types, 8);
            type_index = 0;
        } else {
            Type type = types[type_index];
            if (is_object_or_null(type)) {
                print_addr(get_location(value.offset), fout);
            } else {
                pprint_primitive(type, value, string_pool, fout);
            }
            type_index++;
            if (i != count - 1) fprintf(fout, ", ");
        }
    }
    fprintf(fout, " }");
}

static void print(struct Heap *heap, struct Stack *stack, struct Stack *stack_obj,
        char **string_pool, FILE *fout)
{
    size_t ptr, location, count;
    DelValue dtype = pop(stack);
    Type type = dtype.type;
    if (!is_object_or_null(type)) {
        DelValue dval = pop(stack);
        print_primitive(type, dval, string_pool, fout);
        return;
    }
    ptr = pop(stack_obj).offset;
    location = get_location(ptr);
    count = get_count(ptr);
    if (is_array(type) && type_of_array(type) == TYPE_BYTE) {
        for (size_t i = location; i < count + location; i++) {
            fputc(vector_get(heap->vector, i).byte, stdout);
        }
    } else if (is_array(type)) {
        Type arr_type = type_of_array(type);
        if (!is_object(arr_type)) {
            fprintf(fout, "{ ");
            for (size_t i = location; i < count + location; i++) {
                pprint_primitive(arr_type, vector_get(heap->vector, i), string_pool, fout);
                if (i != count + location - 1) fprintf(fout, ", ");
            }
            fprintf(fout, " }");
        } else {
            fprintf(fout, "{ ");
            for (uint64_t i = location; i < count + location; i++) {
                ptr = vector_get(heap->vector, i).offset;
                print_addr(get_location(ptr), fout);
                if (i != count + location - 1) fprintf(fout, ", ");
            }
            fprintf(fout, " }");
        }
    } else {
        print_object(heap, location, count, string_pool, fout);
    }
}

// TODO: Check that stack does not overflow when pushing
// static inline bool read(struct Stack *stack, struct Heap *heap)//, struct StackFrames *sfs)
// {
//     char packed[8] = {0};
//     uint64_t i = 0;
//     size_t offset = 0;
//     while (true) {
//         int c = getchar();
//         if (c == EOF || c == '\n') {
//             if (c == EOF) clearerr(stdin);
//             break;
//         }
//         packed[offset] = (char) c;
//         if (offset == 7) {
//             push_chars(stack, packed);
//             memset(packed, 0, 8);
//         }
//         i++;
//         offset = i % 8;
//     }
//     // Push the remainder, if we haven't already
//     if (offset != 0) {
//         push_chars(stack, packed);
//     }
//     size_t metadata = i / 8 + (offset == 0 ? 0 : 1);
//     push_offset(stack, offset);
//     push_offset(stack, metadata);
//     return push_heap(heap, stack, NULL);//, sfs);
// }

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
void vm_init(struct VirtualMachine *vm, FILE *fout, FILE *ferr, DelValue *instructions,
        char **string_pool)
{
    vm->fout = fout;
    vm->ferr = ferr;
    vm->stack.values = calloc(STACK_MAX, sizeof(*(vm->stack.values)));
    vm->stack_obj.values = calloc(STACK_MAX, sizeof(*(vm->stack_obj.values)));
    vm->sfs.values = calloc(STACK_MAX, sizeof(*(vm->sfs.values)));
    vm->sfs.frame_offsets = calloc(STACK_MAX, sizeof(*(vm->sfs.frame_offsets)));
    vm->sfs_obj.values = calloc(STACK_MAX, sizeof(*(vm->sfs.values)));
    vm->sfs_obj.frame_offsets = calloc(STACK_MAX, sizeof(*(vm->sfs.frame_offsets)));
    vm->heap.vector = vector_new(HEAP_INIT, HEAP_MAX);
    vm->heap.gc_threshold = GC_GROWTH_FACTOR * vm->heap.vector->capacity;
    vm->instructions = instructions;
    vm->string_pool = string_pool;
}

void vm_free(struct VirtualMachine *vm)
{
    free(vm->stack.values);
    free(vm->stack_obj.values);
    free(vm->sfs.values);
    free(vm->sfs.frame_offsets);
    free(vm->sfs_obj.values);
    free(vm->sfs_obj.frame_offsets);
    vector_free(vm->heap.vector);
}

#if DEBUG_RUNTIME
#define emergency_break() do {\
    if (iterations > 200000) {\
    /* if (iterations > 200) {*/\
        print_stack(&stack, false);\
        print_stack(&stack_obj, true);\
        print_heap(&heap);\
        fprintf(vm->ferr, "infinite loop detected, ending execution\n");\
        status = DEL_VM_STATUS_ERROR;\
        goto exit_loop;\
    }\
} while(0)
#else
#define emergency_break()
#endif

#define pause_after(n) do {\
    if (unexpected(iterations % n == 0)) {\
        status = DEL_VM_STATUS_PAUSE;\
        goto exit_loop;\
    }\
} while(0)

#if DEBUG_RUNTIME
#define debug_print_all() do{\
    print_stack(&stack, false);\
    print_stack(&stack_obj, true);\
    print_frames(&sfs, false);\
    print_frames(&sfs_obj, true);\
    print_heap(&heap);\
    printf("======================================\n");\
} while(0)
#else
#define debug_print_all()
#endif


#define on_break() do {\
    ip++;\
    iterations++;\
    /* For fun, lets exit whenever we reach 100 iterations */ \
    /* pause_after(UINT64_MAX);*/\
    debug_print_all();\
    emergency_break();\
} while(0)

// Bounds check on pushes to stack
#define check_push(stack_ptr) do {\
    if (unexpected((stack_ptr)->offset >= STACK_MAX - 1)) { \
        fprintf(vm->ferr, "Error: stack overflow (calculation too large)\n");\
        status = DEL_VM_STATUS_ERROR;\
        goto exit_loop;\
    }\
} while(0)

static inline bool is_stack_overflow(struct StackFrames *sfs) {
    return sfs->index >= STACK_MAX - 1 || sfs->frame_offsets_index >= STACK_MAX - 1;
}

uint64_t vm_execute(struct VirtualMachine *vm)
{
    // Define local variables for VM fields, for convenience (and maybe efficiency)
    enum DelVirtualMachineStatus status = vm->status;
    struct StackFrames sfs = vm->sfs;
    struct StackFrames sfs_obj = vm->sfs_obj;
    struct Stack stack = vm->stack;
    struct Stack stack_obj = vm->stack_obj;
    struct Heap heap = vm->heap;
    size_t ip = vm->ip;
    uint64_t ret = vm->ret;
    DelValue val1 = vm->val1;
    DelValue val2 = vm->val2;
    size_t iterations = vm->iterations;
    DelValue *instructions = vm->instructions;
    char **string_pool = vm->string_pool;
    size_t count;
    size_t metadata;
#include "threading.h"
    while (1) {
        switch (instructions[ip].opcode) {
            vm_case(PUSH):
                ip++;
                check_push(&stack);
                push(&stack, instructions[ip]);
// #if DEBUG_RUNTIME
//                 print_stack(&stack, false);
// #endif
                vm_break;
            vm_case(PUSH_OBJ):
                ip++;
                check_push(&stack_obj);
                push(&stack_obj, instructions[ip]);
// #if DEBUG_RUNTIME
//                 print_stack(&stack_obj, true);
// #endif
                vm_break;
            vm_case(PUSH_HEAP):
                ip++;
                count = instructions[ip].offset;
                ip++;
                metadata = instructions[ip].offset;
                if (!push_heap(count, metadata, &heap, &stack, &stack_obj, &sfs_obj, string_pool,
                            vm->ferr)) {
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(PUSH_ARRAY):
                if (!push_array(&heap, &stack, &stack_obj, vm->ferr)) {
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(LEN_ARRAY):
                TODO();
                vm_break;
            vm_case(GET_HEAP):
                ip++;
                val1 = pop(&stack_obj);
                if (!get_heap(&heap, instructions[ip].offset, val1.offset, &stack)) {
                    fprintf(vm->ferr, "Error: null pointer exception\n");
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(GET_HEAP_OBJ):
                ip++;
                val1 = pop(&stack_obj);
                if (!get_heap(&heap, instructions[ip].offset, val1.offset, &stack_obj)) {
                    fprintf(vm->ferr, "Error: null pointer exception\n");
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(SET_HEAP):
                ip++;
                val1 = pop(&stack_obj);
                val2 = pop(&stack);
                set_heap(&heap, instructions[ip].offset, val1.offset, val2);
                vm_break;
            vm_case(SET_HEAP_OBJ):
                ip++;
                val1 = pop(&stack_obj);
                val2 = pop(&stack_obj);
                set_heap(&heap, instructions[ip].offset, val1.offset, val2);
                vm_break;
            vm_case(GET_ARRAY):
                val1 = pop(&stack);
                val2 = pop(&stack_obj);
                if (!get_array(val1.integer, val2.offset, &heap, &stack, vm->ferr)) {
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(GET_ARRAY_OBJ):
                val1 = pop(&stack);
                val2 = pop(&stack_obj);
                if (!get_array(val1.integer, val2.offset, &heap, &stack_obj, vm->ferr)) {
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(SET_ARRAY):
                val1 = pop(&stack);
                val2 = pop(&stack_obj);
                if (!set_array(val1.integer, val2.offset, &heap, &stack)) {
                    fprintf(vm->ferr, "Error: array index out of bounds exception\n");
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                vm_break;
            vm_case(SET_ARRAY_OBJ):
                val1 = pop(&stack);
                val2 = pop(&stack_obj);
                if (!set_array(val1.integer, val2.offset, &heap, &stack_obj)) {
                    fprintf(vm->ferr, "Error: array index out of bounds exception\n");
                    status = DEL_VM_STATUS_ERROR;
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
                    fprintf(vm->ferr, "Error: division by zero\n");
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                push_integer(&stack, (val2.integer / val1.integer));
                vm_break;
            vm_case(MOD):
                val1 = pop(&stack);
                val2 = pop(&stack);
                if (val1.integer == 0) {
                    fprintf(vm->ferr, "Error: division by zero\n");
                    status = DEL_VM_STATUS_ERROR;
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
            vm_case(UNARY_MINUS):
                val1 = pop(&stack);
                push_integer(&stack, (-1 * val1.integer));
                vm_break;
            vm_case(EQ_OBJ):
                val1 = pop(&stack_obj);
                val2 = pop(&stack_obj);
                push_offset(&stack, val2.offset == val1.offset);
                vm_break;
            vm_case(NEQ_OBJ):
                val1 = pop(&stack_obj);
                val2 = pop(&stack_obj);
                push_offset(&stack, val2.offset != val1.offset);
                vm_break;
            vm_case(SET_LOCAL):
                ip++;
                set_local(&stack, &sfs, instructions[ip].offset);
                vm_break;
            vm_case(SET_LOCAL_OBJ):
                ip++;
                set_local(&stack_obj, &sfs_obj, instructions[ip].offset);
// #if DEBUG_RUNTIME
//                 print_frames(&sfs, false);
//                 print_frames(&sfs_obj, true);
// #endif
                vm_break;
            vm_case(DEFINE):
                // TODO: Figure out how to make this compile-time only
                sfs.index++;
                vm_break;
            vm_case(DEFINE_OBJ):
                sfs_obj.index++;
                vm_break;
            vm_case(GET_LOCAL):
                ip++;
                val1 = get_local(&sfs, instructions[ip].offset);
                check_push(&stack);
                push(&stack, val1);
                vm_break;
            vm_case(GET_LOCAL_OBJ):
                ip++;
                val1 = get_local(&sfs_obj, instructions[ip].offset);
                check_push(&stack_obj);
                push(&stack_obj, val1);
                vm_break;
            vm_case(JE):
                assert("JE not implemented\n" && false);
                vm_break;
            vm_case(JNE):
                val1 = pop(&stack);
                ip++;
                if (!val1.integer) {
                    ip = instructions[ip].offset;
                    ip--; // reverse the effects of the ip++ in vm_break
                }
                vm_break;
            vm_case(JMP):
                ip++;
                ip = instructions[ip].offset;
                ip--; // reverse the effects of the ip++ in vm_break
                vm_break;
            vm_case(RET):
                ip = pop(&stack).offset;
                ip--;
                vm_break;
            vm_case(POP):
                pop(&stack);
                vm_break;
            vm_case(POP_OBJ):
                pop(&stack_obj);
                vm_break;
            vm_case(EXIT):
                status = DEL_VM_STATUS_COMPLETED;
                goto exit_loop;
            vm_case(CAST_INT):
                val1 = pop(&stack);
                push_integer(&stack, (int64_t)val1.floating);
                vm_break;
            vm_case(CAST_FLOAT):
                val1 = pop(&stack);
                push_floating(&stack, (double)val1.integer);
                vm_break;
            vm_case(CALL):
                ip++;
                uint64_t num_args = instructions[ip].offset;
                union DelForeignValue *dvals = calloc(num_args, sizeof(*dvals));
                for (uint64_t i = 0; i < num_args; i++) {
                    val1 = pop(&stack);
                    memcpy(&(dvals[i]), &val1, sizeof(val1));
                }
                ip++;
                void *context = (void *)instructions[ip].pointer;
                ip++;
                DelForeignFunctionCall fun = (void *)instructions[ip].pointer;
                union DelForeignValue dval = fun(dvals, context);
                // Maybe shouldn't treating everything like an int, but I think it's fine
                push_integer(&stack, dval.integer);
                free(dvals);
                vm_break;
            vm_case(SWAP):
                swap(&stack);
                vm_break;
            vm_case(PUSH_SCOPE):
                if (unexpected(is_stack_overflow(&sfs) || is_stack_overflow(&sfs_obj))) {
                    fprintf(vm->ferr, "Error: stack overflow\n");
                    status = DEL_VM_STATUS_ERROR;
                    goto exit_loop;
                }
                stack_frame_enter(&sfs);
                stack_frame_enter(&sfs_obj);
                vm_break;
            vm_case(POP_SCOPE):
                stack_frame_exit(&sfs);
                stack_frame_exit(&sfs_obj);
                vm_break;
            vm_case(READ):
                assert(false);
                // if (!read(&stack, &heap)) {//, &sfs)) {
                //     status = DEL_VM_STATUS_ERROR;
                //     goto exit_loop;
                // }
                vm_break;
            vm_case(PRINT): {
                print(&heap, &stack, &stack_obj, string_pool, vm->fout);
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
            vm_case(FLOAT_UNARY_MINUS):
                val1 = pop(&stack);
                push_floating(&stack, (-1 * val1.floating));
                vm_break;
            // vm_case(PUSH_STRING):
            default:
                fprintf(vm->ferr, "unknown instruction encountered: '%" PRIu64 "'",
                        instructions[ip].offset);
                status = DEL_VM_STATUS_ERROR;
                goto exit_loop;
        }
        // Note: any statements to be executed before looping back to the top should be done
        // in the on_break macro. When threaded code is enabled, statements below this line
        // will not execute
    }
exit_loop:
#if DEBUG_RUNTIME
    print_stack(&stack, false);
    print_stack(&stack_obj, true);
    print_frames(&sfs, false);
    print_frames(&sfs_obj, true);
    print_heap(&heap);
#endif
    // Update state of VM before exiting
    vm->status = status;
    vm->sfs = sfs;
    vm->sfs_obj = sfs_obj;
    vm->stack = stack;
    vm->stack_obj = stack_obj;
    vm->heap = heap;
    vm->ip = ip;
    vm->ret = ret;
    vm->val1 = val1;
    vm->val2 = val2;
    vm->iterations = iterations;
    vm->instructions = instructions;
    vm->string_pool = string_pool;
    return ret;
}

#undef eval_binary_op
