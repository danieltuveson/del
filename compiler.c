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
    assert("offset is out of bounds\n" && globals->cc->instructions->length < INSTRUCTIONS_MAX - 1);
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

static void compile_heap(struct Globals *globals, size_t metadata, size_t count)
{
    // Load metadata
    push(globals);
    load_offset(globals, metadata);
    // Load count
    push(globals);
    load_offset(globals, count);
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
    add_ft_node(globals, globals->cc->funcall_table, fundef->name, globals->cc->instructions->length);
    compile_funargs(globals, fundef->args);
    compile_statements(globals, fundef->stmts);
}

static void compile_constructor(struct Globals *globals, struct FunCall *funcall)
{
    linkedlist_foreach_reverse(lnode, funcall->args->tail) {
        struct Value *value = lnode->value;
        compile_type(globals, value->type);
        compile_value(globals, value);
    }
    compile_heap(globals, 0, 2 * funcall->args->length);
}

    // load(globals, GET_LOCAL);
    // load(globals, offset);
static void compile_get(struct Globals *globals, struct Accessor *get)
{
    if (linkedlist_is_empty(get->lvalues)) {
        // load(globals, PUSH);
        // load(globals, get->definition->name);
        compile_get_local(globals, get->definition->scope_offset);
        return;
    } 
    // compile_get_local(globals, get->definition->name);
    compile_get_local(globals, get->definition->scope_offset);
    Type parent_value_type = get->definition->type;
    linkedlist_foreach(lnode, get->lvalues->head) {
        struct Class *cls = lookup_class(globals->cc->class_table, parent_value_type);
        struct LValue *lvalue = lnode->value;
        switch (lvalue->lvtype) {
            case LV_PROPERTY: {
                uint64_t index = lookup_property_index(cls, lvalue->property);
                push(globals);
                load_offset(globals, index);
                load_opcode(globals, GET_HEAP);
                parent_value_type = lvalue->type;
                break;
            } default:
                // handle this later
                // struct Value *index;
                printf("Error cannot compile array indexing\n");
                assert(false);
                return;
        }
    }
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
    if (funcall->funname == BUILTIN_PRINT) {
        compile_print(globals, funcall->args, false);
    } else if (funcall->funname == BUILTIN_PRINTLN) {
        compile_print(globals, funcall->args, true);
    } else if (funcall->funname == BUILTIN_READ) {
        load_opcode(globals, READ);
    } else {
        assert("Builtin not implemented" && false);
    }
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
    add_callsite(globals, globals->cc->funcall_table, funcall->funname, next(globals));
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
            compile_constructor(globals, val->funcall);
            break;
        case VTYPE_GET:
            compile_get(globals, val->get);
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

static void compile_set(struct Globals *globals, struct Set *set)
{
    compile_value(globals, set->val);
    if (linkedlist_is_empty(set->to_set->lvalues)) {
        compile_set_local(globals, set->to_set->definition->scope_offset);
        if (set->is_define) load_opcode(globals, DEFINE);
        return;
    } 
    // else {
    //     printf("Error cannot compile property aglobalsess / index\n");
    //     return;
    // }
    // compile_get_local(globals, set->to_set->definition->name);
    compile_get_local(globals, set->to_set->definition->scope_offset);
    Type parent_value_type = set->to_set->definition->type;
    linkedlist_foreach(lnode, set->to_set->lvalues->head) {
        struct Class *cls = lookup_class(globals->cc->class_table, parent_value_type);
        struct LValue *lvalue = lnode->value;
        switch (lvalue->lvtype) {
            case LV_PROPERTY: {
                uint64_t index = lookup_property_index(cls, lvalue->property);
                // NOTE: refactor to not have lookup_property on class, write a generic list lookup function
                if (lnode->next == NULL) {
                    push(globals);
                    load_offset(globals, index);
                    load_opcode(globals, SET_HEAP);
                } else {
                    push(globals);
                    load_offset(globals, index);
                    load_opcode(globals, GET_HEAP);
                }
                parent_value_type = lvalue->type;
                break;
            } default:
                // handle this later
                // struct Value *index;
                printf("Error cannot compile array indexing\n");
                assert(false);
                return;
        }
    }
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
    push(globals);
    load_offset(globals, top_of_loop);
    load_opcode(globals, JMP);
    globals->cc->instructions->values[old_offset].offset = globals->cc->instructions->length; // set JNE jump to go to end of loop
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

static void compile_statement(struct Globals *globals, struct Statement *stmt)
{
    switch (stmt->type) {
        case STMT_SET:
            compile_set(globals, stmt->set);
            break;
        case STMT_RETURN:
            compile_return(globals, stmt->ret);
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

// static int compile_let(struct Globals *globals, Let *let, int offset)
// {
//     for (; let != NULL; let = let->next) {
//         // TODO: check that value type is valid
//         struct Definition *def = (struct Definition *) let->value;
//         load(PUSH);
//         load(0); // Initialize to 0
//         load(PUSH);
//         load(def->name);
//         load(SET_LOCAL);
//     }
//     return offset;
// }

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

