#include "common.h"
#include "linkedlist.h"
#include "ast.h"
#include "typecheck.h"
#include "printers.h"
#include "compiler.h"
#include "vector.h"

static void compile_value(struct Globals *globals, struct Value *val);
static void compile_expr(struct Globals *globals, struct Expr *expr);
static void compile_statement(struct Globals *globals, struct Statement *stmt);
static void compile_statements(struct Globals *globals, Statements *stmts);

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

static inline void push(struct Globals *globals)
{
    load_opcode(globals, PUSH);
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

static void compile_heap(struct Globals *globals, size_t metadata, size_t count)
{
    // Load metadata
    compile_offset(globals, metadata);
    // Load count
    compile_offset(globals, count);
    load_opcode(globals, PUSH_HEAP);
}

static inline void compile_int(struct Globals *globals, int64_t integer)
{
    switch (integer) {
        case 0: 
            load_opcode(globals, PUSH_0);
            break;
        case 1:
            load_opcode(globals, PUSH_1);
            break;
        case 2:
            load_opcode(globals, PUSH_2);
            break;
        case 3:
            load_opcode(globals, PUSH_3);
            break;
        default:
            push(globals);
            DelValue value = { .integer = integer };
            vector_append(&(globals->cc->instructions), value);
    }
}

static inline void compile_chars(struct Globals *globals, char chars[8])
{
    push(globals);
    DelValue value;
    memcpy(value.chars, chars, 8);
    vector_append(&(globals->cc->instructions), value);
}

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

static inline void compile_type(struct Globals *globals, Type type)
{
    push(globals);
    DelValue value = { .type = type };
    vector_append(&(globals->cc->instructions), value);
}

static void compile_get_local(struct Globals *globals, size_t offset)
{
    switch (offset) {
        case 0: 
            load_opcode(globals, GET_LOCAL_0);
            break;
        case 1:
            load_opcode(globals, GET_LOCAL_1);
            break;
        case 2:
            load_opcode(globals, GET_LOCAL_2);
            break;
        case 3:
            load_opcode(globals, GET_LOCAL_3);
            break;
        default:
            load_opcode(globals, GET_LOCAL);
            load_offset(globals, offset);
    }
}

static void compile_set_local(struct Globals *globals, size_t offset)
{
    switch (offset) {
        case 0: 
            load_opcode(globals, SET_LOCAL_0);
            break;
        case 1:
            load_opcode(globals, SET_LOCAL_1);
            break;
        case 2:
            load_opcode(globals, SET_LOCAL_2);
            break;
        case 3:
            load_opcode(globals, SET_LOCAL_3);
            break;
        default:
            load_opcode(globals, SET_LOCAL);
            load_offset(globals, offset);
    }
}

// Pack 8 byte chunks of chars to push onto stack
static void compile_string(struct Globals *globals, char *string)
{
    char packed[8] = {0};
    uint64_t i = 0;
    int offset = 0;
    while (string[i] != '\0') {
        packed[offset] = string[i];
        if (offset == 7) {
            compile_chars(globals, packed);
            memset(packed, 0, 8);
        }
        i++;
        offset = i % 8;
    }
    // Push the remainder, if we haven't already
    if (offset != 0) {
        compile_chars(globals, packed);
    }
    compile_heap(globals, offset, i / 8 + (offset == 0 ? 0 : 1));
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
    linkedlist_foreach(lnode, defs->head) {
        struct Definition *def = lnode->value;
        compile_set_local(globals, def->scope_offset);
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
    add_ft_node(globals, globals->cc->funcall_table, fundef->name,
            globals->cc->instructions->length);
    compile_funargs(globals, fundef->args);
    compile_statements(globals, fundef->stmts);
}

static void compile_constructor(struct Globals *globals, struct Constructor *constructor)
{
    linkedlist_foreach_reverse(lnode, constructor->funcall->args->tail) {
        struct Value *value = lnode->value;
        compile_type(globals, value->type);
        compile_value(globals, value);
    }
    compile_heap(globals, 0, 2 * constructor->funcall->args->length);
}

// Being a little too cheeky with the name of this?
static uint64_t compile_xet_property(struct Globals *globals, struct GetProperty *get,
        bool is_increment)
{
    if (get->type == LV_INDEX) {
        printf("Not implemented\n");
        assert(false);
    }
    compile_value(globals, get->accessor);
    if (is_increment) {
        load_opcode(globals, DUP);
    }
    struct Class *cls = lookup_class(globals->cc->class_table, get->accessor->type);
    uint64_t index = lookup_property_index(cls, get->property);
    compile_offset(globals, index);
    return index;
}

static uint64_t compile_get_property(struct Globals *globals, struct GetProperty *get,
        bool is_increment)
{
    uint64_t index = compile_xet_property(globals, get, is_increment);
    load_opcode(globals, GET_HEAP);
    return index;
}

static void compile_print(struct Globals *globals, Values *args, bool has_newline)
{
    linkedlist_foreach(lnode, args->head) {
        struct Value *value = lnode->value;
        compile_value(globals, value);
        compile_type(globals, value->type);
        load_opcode(globals, PRINT);
    }
    if (has_newline) {
        compile_string(globals, "\n");
        compile_type(globals, TYPE_STRING);
        load_opcode(globals, PRINT);
    }
}

static void compile_builtin_funcall(struct Globals *globals, struct FunCall *funcall)
{
    Symbol funname = funcall->access->definition->name;
    if (funname == BUILTIN_PRINT) {
        compile_print(globals, funcall->args, false);
    } else if (funname == BUILTIN_PRINTLN) {
        compile_print(globals, funcall->args, true);
    } else if (funname == BUILTIN_READ) {
        load_opcode(globals, READ);
    } else {
        assert("Builtin not implemented" && false);
    }
}

// static void compile_array(struct Globals *globals, struct Constructor *constructor)
// {
//     // TODO
//     return;
// }

static void compile_builtin_constructor(struct Globals *globals, struct Constructor *constructor)
{
    assert("Builtin not implemented" && false);
    // if (constructor->funcall->funname != BUILTIN_ARRAY) {
        // assert("Builtin not implemented" && false);
    // }
    // compile_array(globals, constructor);
}

static void compile_funcall(struct Globals *globals, struct FunCall *funcall)
{
    push(globals);
    size_t bookmark = next(globals);
    if (funcall->args != NULL) {
        linkedlist_foreach_reverse(lnode, funcall->args->tail) {
            compile_value(globals, lnode->value);
        }
    }
    load_opcode(globals, PUSH_SCOPE);
    push(globals);
    Symbol funname = funcall->access->definition->name;
    struct FunctionCallTable *fct = globals->cc->funcall_table;
    add_callsite(globals, fct, funname, next(globals));
    load_opcode(globals, JMP);
    globals->cc->instructions->values[bookmark].offset = globals->cc->instructions->length;
    load_opcode(globals, POP_SCOPE);
}

static void compile_value(struct Globals *globals, struct Value *val)
{
    switch (val->vtype) {
        case VTYPE_STRING:
            compile_string(globals, val->string);
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
            compile_int(globals, val->integer);
            break;
        case VTYPE_EXPR:
            compile_expr(globals, val->expr);
            break;
        case VTYPE_FUNCALL:
            compile_funcall(globals, val->funcall); 
            break;
        case VTYPE_BUILTIN_FUNCALL:
            compile_builtin_funcall(globals, val->funcall); 
            break;
        case VTYPE_CONSTRUCTOR:
            compile_constructor(globals, val->constructor);
            break;
        case VTYPE_GET_LOCAL:
            compile_get_local(globals, val->get_local->scope_offset);
            break;
        case VTYPE_GET_PROPERTY:
            compile_get_property(globals, val->get_property, false);
            break;
        case VTYPE_BUILTIN_CONSTRUCTOR:
            compile_builtin_constructor(globals, val->constructor);
            break;
        default: printf("compile not implemented yet\n"); assert(false);
    }
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
           } else {
               compile_binary_op(globals, val1, val2, EQ);
           }
           break;
        case OP_NOT_EQ:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_NEQ);
           } else {
               compile_binary_op(globals, val1, val2, NEQ);
           }
           break;
        case OP_GREATER_EQ:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_GTE);
           } else {
               compile_binary_op(globals, val1, val2, GTE);
           }
           break;
        case OP_GREATER:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_GT);
           } else {
               compile_binary_op(globals, val1, val2, GT);
           }
           break;
        case OP_LESS_EQ:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_LTE);
           } else {
               compile_binary_op(globals, val1, val2, LTE);
           }
           break;
        case OP_LESS:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_LT);
           } else {
               compile_binary_op(globals, val1, val2, LT);
           }
           break;
        case OP_PLUS:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_ADD);
           } else {
               compile_binary_op(globals, val1, val2, ADD);
           }
           break;
        case OP_MINUS:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_SUB);
           } else {
               compile_binary_op(globals, val1, val2, SUB);
           }
           break;
        case OP_STAR:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_MUL);
           } else {
               compile_binary_op(globals, val1, val2, MUL);
           }
           break;
        case OP_SLASH:
           if (val1->type == TYPE_FLOAT) {
               compile_binary_op(globals, val1, val2, FLOAT_DIV);
           } else {
               compile_binary_op(globals, val1, val2, DIV);
           }
           break;
        case OP_PERCENT:
           compile_binary_op(globals, val1, val2, MOD);
           break;
        case OP_UNARY_PLUS:
           if (val1->type == TYPE_FLOAT) {
               compile_unary_op(globals, val1, FLOAT_UNARY_PLUS);
           } else {
               compile_unary_op(globals, val1, UNARY_PLUS);
           }
           break;
        case OP_UNARY_MINUS:
           if (val1->type == TYPE_FLOAT) {
               compile_unary_op(globals, val1, FLOAT_UNARY_MINUS);
           } else {
               compile_unary_op(globals, val1, UNARY_PLUS);
           }
           break;
        default:
           printf("Error cannot compile expression\n");
           assert(false);
           break;
    }
}

static void compile_set_property(struct Globals *globals, struct SetProperty *set)
{
    compile_value(globals, set->expr);
    compile_xet_property(globals, set->access, false);
    load_opcode(globals, SET_HEAP);
}

static void compile_return(struct Globals *globals, struct Value *ret)
{
    if (ret != NULL) {
        compile_value(globals, ret);
        load_opcode(globals, SWAP);
    }
    load_opcode(globals, JMP);
}

static void compile_if(struct Globals *globals, struct IfStatement *stmt)
{
    push(globals);
    size_t if_offset = next(globals);
    compile_value(globals, stmt->condition);
    load_opcode(globals, JNE);
    compile_statements(globals, stmt->if_stmts);

    size_t else_offset = 0;
    if (stmt->else_stmts) {
        push(globals);
        else_offset = next(globals);
        load_opcode(globals, JMP);
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
    size_t top_of_loop = globals->cc->instructions->length;
    push(globals);
    size_t old_offset = next(globals);
    compile_value(globals, cond);
    load_opcode(globals, JNE);
    compile_statements(globals, stmts);
    if (increment != NULL) compile_statement(globals, increment);
    compile_offset(globals, top_of_loop);
    load_opcode(globals, JMP);
    // set JNE jump to go to end of loop
    globals->cc->instructions->values[old_offset].offset = globals->cc->instructions->length;
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
    uint64_t index = 0;
    if (val->vtype == VTYPE_GET_PROPERTY) {
        index = compile_get_property(globals, val->get_property, true);
    } else if (val->vtype == VTYPE_GET_LOCAL) {
        compile_get_local(globals, val->get_local->scope_offset);
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
        load_opcode(globals, SWAP);
        compile_offset(globals, index);
        load_opcode(globals, SET_HEAP);
    } else if (val->vtype == VTYPE_GET_LOCAL) {
        compile_set_local(globals, val->get_local->scope_offset);
    }
}

static void compile_statement(struct Globals *globals, struct Statement *stmt)
{
    int sign = 1;
    switch (stmt->type) {
        case STMT_SET_LOCAL:
            compile_value(globals, stmt->set_local->expr);
            compile_set_local(globals, stmt->set_local->def->scope_offset);
            if (stmt->set_local->is_define) load_opcode(globals, DEFINE);
            break;
        case STMT_SET_PROPERTY:
            compile_set_property(globals, stmt->set_property);
            break;
        case STMT_RETURN:
            compile_return(globals, stmt->val);
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
            compile_funcall(globals, stmt->funcall);
            break;
        case STMT_BUILTIN_FUNCALL:
            compile_builtin_funcall(globals, stmt->funcall);
            break;
        case STMT_LET:
            load_opcode(globals, DEFINE);
            break;
        case STMT_DEC:
            sign = -1;
            /* fallthrough */
        case STMT_INC:
            compile_increment(globals, stmt->val, sign * 1);
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

static void compile_entrypoint(struct Globals *globals, TopLevelDecls *tlds)
{
    linkedlist_foreach(lnode, tlds->head) {
        struct TopLevelDecl *tld = lnode->value;
        if (tld->type == TLD_TYPE_FUNDEF && tld->fundef->name == globals->entrypoint) {
            load_opcode(globals, PUSH_SCOPE);
            compile_statements(globals, tld->fundef->stmts);
            load_opcode(globals, POP_SCOPE);
            load_opcode(globals, EXIT);
            break;
        }
    }
}

static void compile_tld(struct Globals *globals, struct TopLevelDecl *tld)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            // compile_class(globals, tld->cls);
            break;
        case TLD_TYPE_FUNDEF:
            if (tld->fundef->name != globals->entrypoint) {
                compile_fundef(globals, tld->fundef);
            }
            break;
    }
}

static void compile_tlds(struct Globals *globals, TopLevelDecls *tlds)
{
    globals->cc->funcall_table = new_ft(globals, 0);
    compile_entrypoint(globals, tlds);
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
    compile_tlds(globals, tlds);
    resolve_function_declarations(globals->cc->instructions, globals->cc->funcall_table);
    return globals->cc->instructions->length;
    // run_tests();
    // printf("compiler under construction. come back later.\n");
    // exit(0);
}

