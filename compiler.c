#include "common.h"
#include "linkedlist.h"
#include "ast.h"
#include "ffi.h"
#include "typecheck.h"
#include "printers.h"
#include "compiler.h"
#include "vector.h"

static void compile_value(struct Globals *globals, struct Value *val);
static void compile_expr(struct Globals *globals, struct Expr *expr);
static void compile_array_literal(struct Globals *globals, Type type, Values *vals);
static void compile_statement(struct Globals *globals, struct Statement *stmt);
static void compile_statements(struct Globals *globals, Statements *stmts);

// Function for annotating output bytecode
static void add_comment(struct Globals *globals, char *fmt, ...) {
    size_t max_len = 100;
    char *comment = allocator_malloc(globals->allocator, max_len);
    memset(comment, 0, max_len);
    va_list args;
    va_start(args, fmt);
    vsnprintf(comment, max_len, fmt, args);
    va_end(args);
    struct Comment *c = allocator_malloc(globals->allocator, sizeof(*c));
    c->location = globals->cc->instructions->length;
    c->comment = comment;
    linkedlist_append(globals->cc->comments, c);
}

/* Move offset pointer to the empty element. Return the offset of the last element added */
static inline size_t next(struct Globals *globals) {
    assert("offset is out of bounds\n" &&
            globals->cc->instructions->length < INSTRUCTIONS_MAX - 1);
    vector_grow(&(globals->cc->instructions), 1);
    return globals->cc->instructions->length - 1;
}

/* Add instruction to instruction set */
static inline void load_opcode(struct Globals *globals, enum Code opcode)
{
    DelValue value = { .opcode = opcode };
    vector_append(&(globals->cc->instructions), value);
}

static inline void pop(struct Globals *globals)
{
    load_opcode(globals, POP);
}

static inline void pop_obj(struct Globals *globals)
{
    load_opcode(globals, POP_OBJ);
}

static inline void push(struct Globals *globals)
{
    load_opcode(globals, PUSH);
}

static inline void push_obj(struct Globals *globals)
{
    load_opcode(globals, PUSH_OBJ);
}

static inline void load_offset(struct Globals *globals, size_t offset)
{
    DelValue value = { .offset = offset };
    vector_append(&(globals->cc->instructions), value);
}

static inline void compile_offset(struct Globals *globals, size_t offset)
{
    push(globals);
    DelValue value = { .offset = offset };
    vector_append(&(globals->cc->instructions), value);
}

static inline void load_pointer(struct Globals *globals, void *pointer)
{
    DelValue value = { .pointer = (intptr_t)pointer };
    vector_append(&(globals->cc->instructions), value);
}

static void compile_heap(struct Globals *globals, size_t metadata, size_t count)
{
    load_opcode(globals, PUSH_HEAP);
    // Load count
    load_offset(globals, count);
    // Load metadata
    load_offset(globals, metadata);
}

static inline void compile_int(struct Globals *globals, int64_t integer)
{
    push(globals);
    DelValue value = { .integer = integer };
    vector_append(&(globals->cc->instructions), value);
}

static inline void compile_null(struct Globals *globals)
{
    push_obj(globals);
    DelValue value = { .offset = 0 };
    vector_append(&(globals->cc->instructions), value);
}

// static inline void compile_chars(struct Globals *globals, char chars[8])
// {
//     push(globals);
//     DelValue value;
//     memcpy(value.chars, chars, 8);
//     vector_append(&(globals->cc->instructions), value);
// }

static inline void compile_float(struct Globals *globals, double floating)
{
    push(globals);
    DelValue value = { .floating = floating };
    vector_append(&(globals->cc->instructions), value);
}

static inline void compile_bool(struct Globals *globals, int64_t boolean)
{
    compile_int(globals, boolean);
}

static inline void compile_byte(struct Globals *globals, char byte)
{
    push(globals);
    DelValue value = { .byte = byte };
    vector_append(&(globals->cc->instructions), value);
}

static inline void compile_type(struct Globals *globals, Type type)
{
    push(globals);
    DelValue value = { .type = type };
    vector_append(&(globals->cc->instructions), value);
}

static void compile_xet_local(struct Globals *globals, struct Definition *def, enum Code op)
{
    add_comment(globals, "local %s : %s%s%s%s",
            lookup_symbol(globals, def->name),
            lookup_symbol(globals, def->type),
            is_array(def->type) ? "<" : "",
            is_array(def->type) ? lookup_symbol(globals, type_of_array(def->type)) : "",
            is_array(def->type) ? ">" : "");
    load_opcode(globals, op);
    load_offset(globals, def->scope_offset);
}

static void compile_get_local(struct Globals *globals, struct Definition *def)
{
    enum Code op = is_object(def->type) ? GET_LOCAL_OBJ : GET_LOCAL;
    compile_xet_local(globals, def, op);
}

static void compile_set_local(struct Globals *globals, struct Definition *def)
{
    enum Code op = is_object(def->type) ? SET_LOCAL_OBJ : SET_LOCAL;
    compile_xet_local(globals, def, op);
}

// Pack 8 byte chunks of chars to push onto stack
// TODO: Make byte array builtin, probably repurpose this code
// static void compile_string(struct Globals *globals, char *string)
// {
//     char packed[8] = {0};
//     uint64_t i = 0;
//     int offset = 0;
//     while (string[i] != '\0') {
//         packed[offset] = string[i];
//         if (offset == 7) {
//             compile_chars(globals, packed);
//             memset(packed, 0, 8);
//         }
//         i++;
//         offset = i % 8;
//     }
//     // Push the remainder, if we haven't already
//     if (offset != 0) {
//         compile_chars(globals, packed);
//     }
//     compile_heap(globals, offset, i / 8 + (offset == 0 ? 0 : 1));
// }

static size_t add_to_pool(struct Globals *globals, char *string)
{
    size_t len = 0;
    for (len = 0; string[len] != '\0'; len++) ;
    len++; // null byte
    char *str = calloc(len, sizeof(char));
    memcpy(str, string, len);
    globals->cc->string_pool[globals->cc->string_count] = str;
    size_t old_count = globals->cc->string_count++;
    return old_count;
}

static void compile_string(struct Globals *globals, char *string)
{
    for (size_t i = 0; i < globals->cc->string_count; i++) {
        if (strcmp(string, globals->cc->string_pool[i]) == 0) {
            compile_offset(globals, i);
            return;
        }
    }
    size_t index = add_to_pool(globals, string);
    compile_offset(globals, index);
}

static void compile_binary_op(struct Globals *globals, struct Value *val1, struct Value *val2,
       enum Code op) {
    compile_value(globals, val1);
    compile_value(globals, val2);
    load_opcode(globals, op);
}

static void compile_unary_op(struct Globals *globals, struct Value *val, enum Code op) {
    compile_value(globals, val);
    load_opcode(globals, op);
}

static void compile_funargs(struct Globals *globals, Definitions *defs)
{
    linkedlist_foreach_reverse(lnode, defs->tail) {
        struct Definition *def = lnode->value;
        compile_set_local(globals, def);
        load_opcode(globals, is_object(def->type) ? DEFINE_OBJ: DEFINE);
    }
}

/* Pop arguments from stack, use to define function args
 * - Enter new scope
 * - Execute statements
 * - Push return value to stack
 * - Exit new scope (pop off local variables defined in this scope)
 * - Jump back to caller
 */
static void compile_fundef(struct Globals *globals, struct FunDef *fundef)
{
    if (fundef->is_foreign) {
        // Do nothing - handled in function call
    } else {
        add_ft_node(globals, globals->cc->funcall_table, fundef->name,
                globals->cc->instructions->length);
        compile_funargs(globals, fundef->args);
        compile_statements(globals, fundef->stmts);
    }
}

// v | v | t
// Every i % 4 == 0 element stores types of the next 4 values
// types | value | value | value | value | types | value ...
// (But reverse order, since the heap pops them off in the opposite order)
// TODO: Make this just allocate memory. Handle setting initial values in 
// either user defined constructor, or create a compile time constructor
// that doesn't require a separate opcode
// TODO: This evaluates arguments right to left. If we just make constructors a regular 
// function call, that should make it rtl
static void compile_constructor(struct Globals *globals, struct Constructor *constructor)
{
    uint16_t types[4] = {0};
    size_t rem = constructor->funcall->args->length % 4;
    size_t count = 0;
    size_t i = 0;
    // Push after rem, then push every 4th from the head
    linkedlist_foreach_reverse(lnode, constructor->funcall->args->tail) {
        struct Value *value = lnode->value;
        compile_value(globals, value);
        size_t index = count < rem ? rem - i - 1: 3 - i;
        types[index] = (uint16_t)value->type;
        if (index == 0) {
            DelValue dvalue;
            memcpy(dvalue.types, types, 8);
            push(globals);
            vector_append(&(globals->cc->instructions), dvalue);
            memset(types, 0, 8);
            i = 0;
            count++;
        } else {
            i++;
        }
        count++;
    }
    compile_heap(globals, 0, count);
}

// Being a little too cheeky with the name of this?
static uint64_t compile_xet_property(struct Globals *globals, struct GetProperty *get,
        enum Code code, bool is_increment)
{
    compile_value(globals, get->accessor);
    if (is_increment) {
        assert(false); // fix this later
        load_opcode(globals, DUP);
    }
    struct Class *cls = lookup_class(globals->cc->class_table, get->accessor->type);
    uint64_t index = lookup_property_index(cls, get->property);
    load_opcode(globals, code);
    load_offset(globals, 1 + 5 * index / 4);
    return index;
}

static void compile_get_property(struct Globals *globals, Type type, struct GetProperty *get,
        bool is_increment)
{
    if (is_array(get->accessor->type) && get->property == BUILTIN_LENGTH) {
        compile_value(globals, get->accessor);
        load_opcode(globals, LEN_ARRAY);
    } else {
        enum Code code = is_object(type) ? GET_HEAP_OBJ : GET_HEAP;
        compile_xet_property(globals, get, code, is_increment);
        // load_opcode(globals, GET_HEAP);
        // return index;
    }
}

static void compile_xet_index(struct Globals *globals, struct GetProperty *get, bool is_increment)
{
    compile_value(globals, get->accessor);
    if (is_increment) {
        load_opcode(globals, DUP);
    }
    compile_value(globals, get->index);
    if (is_increment) {
        // This won't work, need to swap at specific index
        assert(false);
        load_opcode(globals, SWAP);
        load_opcode(globals, DUP);
    }
}

static void compile_get_index(struct Globals *globals, Type type, struct GetProperty *get,
        bool is_increment)
{
    compile_xet_index(globals, get, is_increment);
    load_opcode(globals, is_object(type) ? GET_ARRAY_OBJ : GET_ARRAY);
}

static bool is_int(Type type)
{
    return type == TYPE_INT || type == TYPE_BYTE || type == TYPE_BOOL;
}

static void compile_cast(struct Globals *globals, struct Cast *cast)
{
    compile_value(globals, cast->value);
    if (is_int(cast->type) && cast->value->type == TYPE_FLOAT) {
        load_opcode(globals, CAST_INT);
    } else if (cast->type == TYPE_FLOAT && is_int(cast->value->type)) {
        load_opcode(globals, CAST_FLOAT);
    } else if (is_int(cast->value->type) && is_int(cast->type)) {
        // Don't need to do anything for int-int conversions
    } else if (cast->value->type == TYPE_STRING && is_array(cast->type)
            && type_of_array(cast->type)) {
        load_opcode(globals, CAST_BYTE_ARRAY);
    } else {
        assert(false);
    }
}

static void compile_print(struct Globals *globals, Values *args)
{
    linkedlist_foreach(lnode, args->head) {
        struct Value *value = lnode->value;
        compile_value(globals, value);
        compile_type(globals, value->type);
        load_opcode(globals, PRINT);
    }
}

static void compile_builtin_funcall(struct Globals *globals, struct FunCall *funcall)
{
    Symbol funname = funcall->access->definition->name;
    if (funname == BUILTIN_PRINT || funname == BUILTIN_PRINTLN) {
        compile_print(globals, funcall->args);
    } else if (funname == BUILTIN_READ) {
        load_opcode(globals, READ);
    } else {
        assert("Builtin not implemented" && false);
    }
}

static void compile_array(struct Globals *globals, Type type, struct Constructor *constructor)
{
    compile_value(globals, constructor->funcall->args->head->value);
    compile_type(globals, type_of_array(type));
    load_opcode(globals, PUSH_ARRAY);
}

static void compile_builtin_constructor(struct Globals *globals, Type type,
        struct Constructor *constructor)
{
    if (constructor->funcall->access->definition->name != BUILTIN_ARRAY) {
        assert("Builtin not implemented" && false);
    }
    compile_array(globals, type, constructor);
}

static void compile_funcall_args(struct Globals *globals, struct LinkedList *args)
{
    if (args != NULL) {
        linkedlist_foreach(lnode, args->head) {
            compile_value(globals, lnode->value);
        }
    }
}

static void compile_foreign_funcall(struct Globals *globals, struct FunCall *funcall, bool is_stmt)
{
    Symbol funname = funcall->access->definition->name;
    struct FunDef *fundef = lookup_fun(globals->cc->fundef_table, funname);
    add_comment(globals, "foreign function call: %s", lookup_symbol(globals, funname));
    size_t num_args = 0;
    if (funcall->args != NULL) {
        struct Value *value = NULL;
        linkedlist_vforeach_reverse(value, funcall->args) {
            compile_value(globals, value);
            num_args++;
        }
    }
    load_opcode(globals, CALL);
    load_offset(globals, num_args);
    load_pointer(globals, fundef->ffb->context);
    load_pointer(globals, fundef->ffb->function);
    if (is_stmt) {
        pop(globals);
    }
    if (fundef->ffb->is_yielding) {
        load_opcode(globals, YIELD);
    }
}

static void compile_del_funcall(struct Globals *globals, struct FunCall *funcall, bool is_stmt)
{
    Symbol funname = funcall->access->definition->name;
    struct FunDef *fundef = lookup_fun(globals->cc->fundef_table, funname);
    add_comment(globals, "function call: %s", lookup_symbol(globals, funname));
    push(globals);
    size_t bookmark = next(globals);
    compile_funcall_args(globals, funcall->args);
    load_opcode(globals, PUSH_SCOPE);
    // push(globals);
    load_opcode(globals, JMP);
    struct FunctionCallTable *fct = globals->cc->funcall_table;
    add_callsite(globals, fct, funname, next(globals));
    globals->cc->instructions->values[bookmark].offset = globals->cc->instructions->length;
    load_opcode(globals, POP_SCOPE);
    if (is_stmt && fundef->rettype != TYPE_UNDEFINED) {
        if (is_object(fundef->rettype)) {
            pop_obj(globals);
        } else {
            pop(globals);
        }
    }
}

static void compile_funcall(struct Globals *globals, struct FunCall *funcall, bool is_stmt)
{
    Symbol funname = funcall->access->definition->name;
    struct FunDef *fundef = lookup_fun(globals->cc->fundef_table, funname);
    if (fundef->is_foreign) {
        compile_foreign_funcall(globals, funcall, is_stmt);
    } else {
        compile_del_funcall(globals, funcall, is_stmt);
    }
}

static void compile_value(struct Globals *globals, struct Value *val)
{
    switch (val->vtype) {
        case VTYPE_STRING:
            compile_string(globals, val->string);
            break;
        case VTYPE_BYTE:
            compile_byte(globals, val->byte);
            break;
        case VTYPE_INT:
            compile_int(globals, val->integer);
            break;
        case VTYPE_FLOAT:
            compile_float(globals, val->floating);
            break;
        case VTYPE_BOOL:
            compile_bool(globals, val->boolean);
            break;
        case VTYPE_NULL:
            compile_null(globals);
            break;
        case VTYPE_EXPR:
            compile_expr(globals, val->expr);
            break;
        case VTYPE_ARRAY_LITERAL:
            compile_array_literal(globals, type_of_array(val->type), val->array);
            break;
        case VTYPE_FUNCALL:
            compile_funcall(globals, val->funcall, false); 
            break;
        case VTYPE_BUILTIN_FUNCALL:
            compile_builtin_funcall(globals, val->funcall); 
            break;
        case VTYPE_CONSTRUCTOR:
            compile_constructor(globals, val->constructor);
            break;
        case VTYPE_GET_LOCAL:
            compile_get_local(globals, val->get_local);
            break;
        case VTYPE_GET_PROPERTY:
            compile_get_property(globals, val->type, val->get_property, false);
            break;
        case VTYPE_INDEX:
            compile_get_index(globals, val->type, val->get_property, false);
            break;
        case VTYPE_CAST:
            compile_cast(globals, val->cast);
            break;
        case VTYPE_BUILTIN_CONSTRUCTOR:
            compile_builtin_constructor(globals, val->type, val->constructor);
            break;
    }
}

static inline bool is_int_type(struct Value *val)
{
    return val->type == TYPE_INT || val->type == TYPE_BYTE;
}

static void compile_expr(struct Globals *globals, struct Expr *expr)
{
    struct Value *val1, *val2;
    val1 = expr->val1;
    val2 = expr->val2;
    switch (expr->op) {
        case OP_OR:
           compile_binary_op(globals, val1, val2, OR);
           break;
        case OP_AND:
           compile_binary_op(globals, val1, val2, AND);
           break;
        case OP_EQEQ:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_EQ);
           } else if (is_object_or_null(val1->type)) {
               compile_binary_op(globals, val1, val2, EQ_OBJ);
           } else {
               compile_binary_op(globals, val1, val2, EQ);
           }
           break;
        case OP_NOT_EQ:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_NEQ);
           } else if (is_object_or_null(val1->type)) {
               compile_binary_op(globals, val1, val2, NEQ_OBJ);
           } else {
               compile_binary_op(globals, val1, val2, NEQ);
           }
           break;
        case OP_GREATER_EQ:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_GTE);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, GTE);
           }
           break;
        case OP_GREATER:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_GT);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, GT);
           }
           break;
        case OP_LESS_EQ:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_LTE);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, LTE);
           }
           break;
        case OP_LESS:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_LT);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, LT);
           }
           break;
        case OP_PLUS:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_ADD);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, ADD);
           }
           break;
        case OP_MINUS:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_SUB);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, SUB);
           }
           break;
        case OP_STAR:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_MUL);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, MUL);
           }
           break;
        case OP_SLASH:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_DIV);
           } else if (is_int_type(val1)) {
               compile_binary_op(globals, val1, val2, DIV);
           }
           break;
        case OP_PERCENT:
           compile_binary_op(globals, val1, val2, MOD);
           break;
        case OP_UNARY_PLUS:
           // Why do languages even have this
           break;
        case OP_UNARY_MINUS:
           if (val1->type == TYPE_FLOAT) {
               compile_unary_op(globals, val1, FLOAT_UNARY_MINUS);
           } else if (val1->type == TYPE_INT) {
               compile_unary_op(globals, val1, UNARY_MINUS);
           }
           break;
        case OP_UNARY_NOT:
           compile_unary_op(globals, val1, NOT);
           break;
        default:
           printf("Error cannot compile expression\n");
           assert(false);
           break;
    }
}

static void compile_array_literal(struct Globals *globals, Type type, Values *vals)
{
    compile_int(globals, vals->length);
    compile_type(globals, type);
    load_opcode(globals, PUSH_ARRAY);

    enum Code code = is_object_or_null(type) ? SET_ARRAY_OBJ : SET_ARRAY;
    struct Value *val = NULL;
    int64_t i = 0;
    linkedlist_vforeach(val, vals) {
        load_opcode(globals, DUP_OBJ);
        compile_value(globals, val);
        if (is_object_or_null(type)) {
            load_opcode(globals, SWAP_OBJ);
        }
        compile_int(globals, i);
        load_opcode(globals, code);
        i++;
    } 
}

static void compile_set_property(struct Globals *globals, struct SetProperty *set)
{
    compile_value(globals, set->expr);
    enum Code code = is_object(set->expr->type) ? SET_HEAP_OBJ : SET_HEAP;
    compile_xet_property(globals, set->access, code, false);
}

static void compile_set_index(struct Globals *globals, struct SetProperty *set)
{
    compile_value(globals, set->expr);
    compile_xet_index(globals, set->access, false);
    load_opcode(globals, is_object(set->expr->type) ? SET_ARRAY_OBJ : SET_ARRAY);
}

static size_t *compile_exit(struct Globals *globals)
{
    size_t *bookmark = allocator_malloc(globals->allocator, sizeof(size_t));
    // push(globals);
    // *bookmark = next(globals);
    load_opcode(globals, JMP);
    *bookmark = next(globals);
    return bookmark;
}

static void compile_return(struct Globals *globals, struct Value *ret)
{
    if (ret != NULL) {
        compile_value(globals, ret);
        if (!is_object(ret->type)) {
            load_opcode(globals, SWAP);
        }
    }
    load_opcode(globals, RET);
}

static void compile_break(struct Globals *globals)
{
    linkedlist_append(globals->cc->breaks, compile_exit(globals));
}

static void compile_continue(struct Globals *globals)
{
    linkedlist_append(globals->cc->continues, compile_exit(globals));
}

static void compile_if(struct Globals *globals, struct IfStatement *stmt)
{
    // push(globals);
    // size_t if_offset = next(globals);
    compile_value(globals, stmt->condition);
    load_opcode(globals, JNE);
    size_t if_offset = next(globals);
    compile_statements(globals, stmt->if_stmts);

    size_t else_offset = 0;
    if (stmt->else_stmts) {
        // push(globals);
        // else_offset = next(globals);
        load_opcode(globals, JMP);
        else_offset = next(globals);
    }

    // set JNE jump to go to after if statement
    globals->cc->instructions->values[if_offset].offset = globals->cc->instructions->length;

    if (stmt->else_stmts) {
        compile_statements(globals, stmt->else_stmts);
        // set JMP jump to go to after else statement when if statement is true
        globals->cc->instructions->values[else_offset].offset = globals->cc->instructions->length;
    }
}

static void compile_loop(struct Globals *globals, struct Value *cond,
        Statements *stmts, struct Statement *increment)
{
    // Save existing break/continues for outer loop
    BreakLocations *breaks = globals->cc->breaks;
    BreakLocations *continues = globals->cc->continues;
    globals->cc->breaks = linkedlist_new(globals->allocator);
    globals->cc->continues = linkedlist_new(globals->allocator);
    // Loop
    size_t top_of_loop = globals->cc->instructions->length;
    // push(globals);
    // size_t old_offset = next(globals);
    compile_value(globals, cond);
    load_opcode(globals, JNE);
    size_t old_offset = next(globals);
    compile_statements(globals, stmts);
    size_t continue_loc = globals->cc->instructions->length;
    if (increment != NULL) compile_statement(globals, increment);
    // compile_offset(globals, top_of_loop);
    load_opcode(globals, JMP);
    load_offset(globals, top_of_loop);
    // Set JNE to the loop exit
    size_t end_of_loop = globals->cc->instructions->length;
    globals->cc->instructions->values[old_offset].offset = end_of_loop;
    // Set any break statements to exit the loop
    linkedlist_foreach(lnode, globals->cc->breaks->head) {
        size_t *loc = lnode->value;
        globals->cc->instructions->values[*loc].offset = end_of_loop;
    }
    // Set any continue statements to jump to increment/jump back to top
    linkedlist_foreach(lnode, globals->cc->continues->head) {
        size_t *loc = lnode->value;
        globals->cc->instructions->values[*loc].offset = continue_loc;
    }
    // Restore outer loop break/continues
    globals->cc->breaks = breaks;
    globals->cc->continues = continues;
}

static void compile_while(struct Globals *globals, struct While *while_stmt)
{
    compile_loop(globals, while_stmt->condition, while_stmt->stmts, NULL);
}

static void compile_for(struct Globals *globals, struct For *for_stmt)
{
    compile_statement(globals, for_stmt->init);
    compile_loop(globals, for_stmt->condition, for_stmt->stmts, for_stmt->increment);
}

static void compile_increment(struct Globals *globals, struct Value *val, int64_t num)
{
    // uint64_t index = 0;
    if (val->vtype == VTYPE_GET_PROPERTY) {
        TODO();
        // index = compile_get_property(globals, val->get_property, true);
    } else if (val->vtype == VTYPE_GET_LOCAL) {
        compile_get_local(globals, val->get_local);
    } else {
        assert(false);
    }
    if (val->type == TYPE_INT) {
        compile_int(globals, num);
        load_opcode(globals, ADD);
    } else if (val->type == TYPE_FLOAT) {
        compile_float(globals, (double)num);
        load_opcode(globals, FLOAT_ADD);
    } else {
        assert(false);
    }
    if (val->vtype == VTYPE_GET_PROPERTY) {
        TODO();
        // load_opcode(globals, SWAP);
        // compile_offset(globals, index);
        // load_opcode(globals, SET_HEAP);
    } else if (val->vtype == VTYPE_GET_LOCAL) {
        compile_set_local(globals, val->get_local);
    }
}

static void compile_statement(struct Globals *globals, struct Statement *stmt)
{
    switch (stmt->type) {
        case STMT_SET_LOCAL:
            compile_value(globals, stmt->set_local->expr);
            compile_set_local(globals, stmt->set_local->def);
            if (stmt->set_local->is_define) {
                load_opcode(globals, is_object(stmt->set_local->def->type) ? DEFINE_OBJ: DEFINE);
            }
            break;
        case STMT_SET_PROPERTY:
            compile_set_property(globals, stmt->set_property);
            break;
        case STMT_SET_INDEX:
            compile_set_index(globals, stmt->set_property);
            break;
        case STMT_RETURN:
            compile_return(globals, stmt->val);
            break;
        case STMT_BREAK:
            compile_break(globals);
            break;
        case STMT_CONTINUE:
            compile_continue(globals);
            break;
        case STMT_IF:
            compile_if(globals, stmt->if_stmt);
            break;
        case STMT_WHILE:
            compile_while(globals, stmt->while_stmt);
            break;
        case STMT_FOR:
            compile_for(globals, stmt->for_stmt);
            break;
        case STMT_FUNCALL:
            compile_funcall(globals, stmt->funcall, true);
            break;
        case STMT_BUILTIN_FUNCALL:
            compile_builtin_funcall(globals, stmt->funcall);
            break;
        case STMT_LET:
            linkedlist_foreach(lnode, stmt->let->head) {
                struct Definition *def = lnode->value;
                load_opcode(globals, is_object(def->type) ? DEFINE_OBJ: DEFINE);
            }
            break;
        case STMT_DEC:
            compile_increment(globals, stmt->val, -1);
            break;
        case STMT_INC:
            compile_increment(globals, stmt->val, 1);
            break;
        default:
            assert("Error cannot compile statement type: not implemented\n" && false);
            break;
    }
}

static void compile_statements(struct Globals *globals, Statements *stmts)
{
    linkedlist_foreach(lnode, stmts->head) {
        compile_statement(globals, lnode->value);
    }
}

// static void compile_class(struct Globals *globals, struct Class *cls)
// {
//     compile_constructor(globals, cls);
// }

static void compile_entrypoint(struct Globals *globals)
{
    Symbol funname = globals->entrypoint;
    struct Accessor *a = new_accessor(globals, funname, linkedlist_new(globals->allocator));
    struct Statement *stmt = new_sfuncall(globals, a, NULL, false);
    compile_funcall(globals, stmt->funcall, true);
    load_opcode(globals, EXIT);
}

static void compile_tld(struct Globals *globals, struct TopLevelDecl *tld)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            // compile_class(globals, tld->cls);
            break;
        case TLD_TYPE_FUNDEF:
            if (!tld->fundef->is_foreign) {
                add_comment(globals, "function definition: %s",
                        lookup_symbol(globals, tld->fundef->name));
            }
            compile_fundef(globals, tld->fundef);
            break;
    }
}

static void compile_tlds(struct Globals *globals, TopLevelDecls *tlds)
{
    globals->cc->funcall_table = new_ft(globals, 0);
    compile_entrypoint(globals);
    linkedlist_foreach(lnode, tlds->head) {
        compile_tld(globals, (struct TopLevelDecl *) lnode->value);
    }
}

// Looks through compiled bytecode and adds references to where function is defined
static void resolve_function_declarations_help(struct Vector *instructions,
    struct FunctionCallTableNode *fn)
{
    linkedlist_foreach(lnode, fn->callsites->head) {
        uint64_t *callsite = lnode->value;
        // printf("callsite %" PRIu64 " updated with function %s at location %" PRIu64,
        //         *callsite, lookup_symbol(fn->function), fn->location);
        instructions->values[*callsite].offset = fn->location;
    }
}

static void resolve_function_declarations(struct Vector *instructions,
        struct FunctionCallTable *funcall_table)
{
    if (funcall_table == NULL) return;
    resolve_function_declarations_help(instructions, funcall_table->node);
    resolve_function_declarations(instructions, funcall_table->left);
    resolve_function_declarations(instructions, funcall_table->right);
    return;
}

// #include "test_compile.c"

size_t compile(struct Globals *globals, TopLevelDecls *tlds)
{
    globals->cc->instructions = vector_new(128, INSTRUCTIONS_MAX);
    globals->cc->comments     = linkedlist_new(globals->allocator);
    globals->cc->breaks       = linkedlist_new(globals->allocator);
    globals->cc->continues    = linkedlist_new(globals->allocator);
    globals->cc->string_count = 0;
    if (globals->string_count > 0) {
        globals->cc->string_pool = calloc(globals->string_count, sizeof(char *));
    } else {
        globals->cc->string_pool = NULL;
    }
    compile_tlds(globals, tlds);
    resolve_function_declarations(globals->cc->instructions, globals->cc->funcall_table);
    return globals->cc->instructions->length;
    // run_tests();
    // printf("compiler under construction. come back later.\n");
    // exit(0);
}

