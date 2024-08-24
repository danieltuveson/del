#include "common.h"
#include "linkedlist.h"
#include "ast.h"
#include "typecheck.h"
#include "printers.h"
#include "compiler.h"

static void compile_value(struct CompilerContext *cc, struct Value *val);
static void compile_expr(struct CompilerContext *cc, struct Expr *expr);
static void compile_statement(struct CompilerContext *cc, struct Statement *stmt);
static void compile_statements(struct CompilerContext *cc, Statements *stmts);

/* Move offset pointer to the empty element. Return the offset of the last element added */
static inline size_t next(struct CompilerContext *cc) {
    assert("offset is out of bounds\n" && cc->offset < INSTRUCTIONS_SIZE - 1);
    return cc->offset++;
}

/* Add instruction to instruction set */
static inline void load_opcode(struct CompilerContext *cc, enum Code opcode)
{
    cc->instructions[next(cc)].opcode = opcode;
}

static inline void push(struct CompilerContext *cc)
{
    load_opcode(cc, PUSH);
}

static inline void load_offset(struct CompilerContext *cc, size_t offset)
{
    cc->instructions[next(cc)].offset = offset;
}
// typedef union {
//     int64_t integer;
//     size_t offset;
//     double floating;
//     char character;
//     enum Code opcode;
// } DelValue;

static inline void compile_int(struct CompilerContext *cc, int64_t integer)
{
    push(cc);
    cc->instructions[next(cc)].integer = integer;
}

static inline void compile_float(struct CompilerContext *cc, double floating)
{
    assert("floating point not implemented\n" && 0);
    push(cc);
    cc->instructions[next(cc)].floating = floating;
}

static inline void compile_bool(struct CompilerContext *cc, int64_t boolean)
{
    compile_int(cc, boolean);
}

static void compile_loadsym(struct CompilerContext *cc, size_t offset)
// static void compile_loadsym(struct CompilerContext *cc, Symbol symbol)
{
    load_opcode(cc, GET_LOCAL);
    load_offset(cc, offset);
}

// Pack 8 byte chunks of chars into longs to push onto stack
static void compile_string(struct CompilerContext *cc, char *string)
{
    assert("Error: not implemented\n" && false);
    // uint64_t packed = 0;
    // uint64_t i = 0;
    // uint64_t tmp;
    // for (; string[i] != '\0'; i++) {
    //     tmp = (uint64_t) string[i];
    //     switch ((i + 1) % 8) {
    //         case 0:
    //             packed = packed | (tmp << 56);
    //             push(cc);
    //             load(cc, packed);
    //             break;
    //         case 1: packed = tmp    | (tmp << 0);  break; /* technically "tmp << 0" does nothing */
    //         case 2: packed = packed | (tmp << 8);  break;
    //         case 3: packed = packed | (tmp << 16); break;
    //         case 4: packed = packed | (tmp << 24); break;
    //         case 5: packed = packed | (tmp << 32); break;
    //         case 6: packed = packed | (tmp << 40); break;
    //         case 7: packed = packed | (tmp << 48); break;
    //     }
    // }
    // // If string is not multiple of 8 bytes, push the remainder
    // if ((i + 1) % 8 != 0) {
    //     push(cc);
    //     load(cc, packed);
    //     // push count
    //     push(cc);
    //     load(cc, i / 8 + 1);
    // } else {
    //     push(cc);
    //     load(cc, i / 8);
    // }
    // load_opcode(cc, PUSH_HEAP);
}

static void compile_binary_op(struct CompilerContext *cc, struct Value *val1, struct Value *val2,
       enum Code op) {
    compile_value(cc, val1);
    compile_value(cc, val2);
    load_opcode(cc, op);
}

static void compile_unary_op(struct CompilerContext *cc, struct Value *val, enum Code op) {
    compile_value(cc, val);
    load_opcode(cc, op);
}

static void compile_funargs(struct CompilerContext *cc, Definitions *defs)
{
    linkedlist_foreach(lnode, defs->head) {
        struct Definition *def = lnode->value;
        push(cc);
        // load(cc, def->name);
        load_offset(cc, def->scope_offset);
        printf("compile_funargs: %s, %ld\n", lookup_symbol(def->name), def->scope_offset);
        load_opcode(cc, SET_LOCAL);
    }
}

/* Pop arguments from stack, use to define function args
 * - Enter new scope
 * - Execute statements
 * - Push return value to stack
 * - Exit new scope (pop off local variables defined in this scope)
 * - Jump back to caller
 */
static void compile_fundef(struct CompilerContext *cc, struct FunDef *fundef)
{
    add_ft_node(cc->funcall_table, fundef->name, cc->offset);
    compile_funargs(cc, fundef->args);
    compile_statements(cc, fundef->stmts);
}

static void compile_constructor(struct CompilerContext *cc, struct FunCall *funcall)
{
    linkedlist_foreach_reverse(lnode, funcall->args->tail) {
        printf("...compiling constructor...\n");
        print_value(lnode->value);
        printf("\n");
        compile_value(cc, lnode->value);
    }
    push(cc);
    load_offset(cc, funcall->args->length);
    load_opcode(cc, PUSH_HEAP);
}

    // load(cc, GET_LOCAL);
    // load(cc, offset);
static void compile_get(struct CompilerContext *cc, struct Accessor *get)
{
    printf("compile_get: %s, %ld\n", lookup_symbol(get->definition->name), get->definition->scope_offset);
    if (linkedlist_is_empty(get->lvalues)) {
        // load(cc, PUSH);
        // load(cc, get->definition->name);
        load_opcode(cc, GET_LOCAL);
        load_offset(cc, get->definition->scope_offset);
        return;
    } 
    // compile_loadsym(cc, get->definition->name);
    compile_loadsym(cc, get->definition->scope_offset);
    Type parent_value_type = get->definition->type;
    linkedlist_foreach(lnode, get->lvalues->head) {
        struct Class *cls = lookup_class(cc->class_table, parent_value_type);
        struct LValue *lvalue = lnode->value;
        switch (lvalue->lvtype) {
            case LV_PROPERTY: {
                uint64_t index = lookup_property_index(cls, lvalue->property);
                push(cc);
                load_offset(cc, index);
                load_opcode(cc, GET_HEAP);
                parent_value_type = lvalue->type;
                break;
            } default:
                // handle this later
                // struct Value *index;
                printf("Error cannot compile array indexing\n");
                return;
        }
    }
}

static void compile_funcall(struct CompilerContext *cc, struct FunCall *funcall)
{
    push(cc);
    size_t bookmark = next(cc);
    if (funcall->args != NULL) {
        linkedlist_foreach_reverse(lnode, funcall->args->tail) {
            // struct Value *val = lnode->value;
            // printf("compiling arg %s...\n", lookup_symbol(val->symbol));
            compile_value(cc, lnode->value);
        }
    }
    load_opcode(cc, PUSH_SCOPE);
    push(cc);
    add_callsite(cc->funcall_table, funcall->funname, next(cc));
    load_opcode(cc, JMP);
    cc->instructions[bookmark].offset = cc->offset;
    load_opcode(cc, POP_SCOPE);
}

static void compile_value(struct CompilerContext *cc, struct Value *val)
{
    switch (val->vtype) {
        case VTYPE_STRING:      compile_string(cc,      val->string);   break;
        case VTYPE_INT:         compile_int(cc,         val->integer);  break;
        case VTYPE_FLOAT:       compile_float(cc,       val->floating); break;
        case VTYPE_BOOL:        compile_bool(cc,        val->boolean);  break;
        // case VTYPE_SYMBOL:      compile_loadsym(cc,     val->symbol);   break;
        case VTYPE_EXPR:        compile_expr(cc,        val->expr);     break;
        case VTYPE_FUNCALL:     compile_funcall(cc,     val->funcall);  break;
        case VTYPE_CONSTRUCTOR: compile_constructor(cc, val->funcall);  break;
        case VTYPE_GET:         compile_get(cc,         val->get);      break;
        // default: printf("compile not implemented yet\n"); assert(0);
    }
}

/* Scrunched up because it's boring */
static void compile_expr(struct CompilerContext *cc, struct Expr *expr)
{
    struct Value *val1, *val2;
    val1 = expr->val1;
    val2 = expr->val2;
    switch (expr->op) {
        case OP_OR:          compile_binary_op(cc, val1, val2, OR);   break;
        case OP_AND:         compile_binary_op(cc, val1, val2, AND);  break;
        case OP_EQEQ:        compile_binary_op(cc, val1, val2, EQ);   break;
        case OP_NOT_EQ:      compile_binary_op(cc, val1, val2, NEQ);  break;
        case OP_GREATER_EQ:  compile_binary_op(cc, val1, val2, GTE);  break;
        case OP_GREATER:     compile_binary_op(cc, val1, val2, GT);   break;
        case OP_LESS_EQ:     compile_binary_op(cc, val1, val2, LTE);  break;
        case OP_LESS:        compile_binary_op(cc, val1, val2, LT);   break;
        case OP_PLUS:        compile_binary_op(cc, val1, val2, ADD);  break;
        case OP_MINUS:       compile_binary_op(cc, val1, val2, SUB);  break;
        case OP_STAR:        compile_binary_op(cc, val1, val2, MUL);  break;
        case OP_SLASH:       compile_binary_op(cc, val1, val2, DIV);  break;
        case OP_PERCENT:     compile_binary_op(cc, val1, val2, MOD);  break;
        case OP_UNARY_PLUS:  compile_unary_op(cc, val1, UNARY_PLUS);  break;
        case OP_UNARY_MINUS: compile_unary_op(cc, val1, UNARY_MINUS); break;
        default: printf("Error cannot compile expression\n"); break;
    }
}

static void compile_set(struct CompilerContext *cc, struct Set *set)
{
    printf("compile_set: %s, %ld\n", lookup_symbol(set->to_set->definition->name), set->to_set->definition->scope_offset);
    compile_value(cc, set->val);
    if (linkedlist_is_empty(set->to_set->lvalues)) {
        push(cc);
        // load(cc, set->to_set->definition->name);
        load_offset(cc, set->to_set->definition->scope_offset);
        load_opcode(cc, SET_LOCAL);
        if (set->is_define) load_opcode(cc, DEFINE);
        return;
    } 
    // else {
    //     printf("Error cannot compile property access / index\n");
    //     return;
    // }
    // compile_loadsym(cc, set->to_set->definition->name);
    compile_loadsym(cc, set->to_set->definition->scope_offset);
    Type parent_value_type = set->to_set->definition->type;
    linkedlist_foreach(lnode, set->to_set->lvalues->head) {
        struct Class *cls = lookup_class(cc->class_table, parent_value_type);
        struct LValue *lvalue = lnode->value;
        switch (lvalue->lvtype) {
            case LV_PROPERTY: {
                uint64_t index = lookup_property_index(cls, lvalue->property);
                // NOTE: refactor to not have lookup_property on class, write a generic list lookup function
                if (lnode->next == NULL) {
                    push(cc);
                    load_offset(cc, index);
                    load_opcode(cc, SET_HEAP);
                } else {
                    push(cc);
                    load_offset(cc, index);
                    load_opcode(cc, GET_HEAP);
                }
                parent_value_type = lvalue->type;
                break;
            } default:
                // handle this later
                // struct Value *index;
                printf("Error cannot compile array indexing\n");
                return;
        }
    }
}

static void compile_return(struct CompilerContext *cc, struct Value *ret)
{
    if (ret != NULL) {
        compile_value(cc, ret);
        load_opcode(cc, SWAP);
    }
    load_opcode(cc, JMP);
}

static void compile_if(struct CompilerContext *cc, struct IfStatement *stmt)
{
    push(cc);
    size_t old_offset = next(cc);
    compile_value(cc, stmt->condition);
    load_opcode(cc, JNE);
    compile_statements(cc, stmt->if_stmts);

    // set JNE jump to go to after if statement
    cc->instructions[old_offset].offset = cc->offset;

    if (stmt->else_stmts) {
        compile_statements(cc, stmt->else_stmts);
    }
}

static void compile_loop(struct CompilerContext *cc, struct Value *cond,
        Statements *stmts, struct Statement *increment)
{
    size_t top_of_loop = cc->offset;
    push(cc);
    size_t old_offset = next(cc);
    compile_value(cc, cond);
    load_opcode(cc, JNE);
    compile_statements(cc, stmts);
    if (increment != NULL) compile_statement(cc, increment);
    push(cc);
    load_offset(cc, top_of_loop);
    load_opcode(cc, JMP);
    cc->instructions[old_offset].offset = cc->offset; // set JNE jump to go to end of loop
}

static void compile_while(struct CompilerContext *cc, struct While *while_stmt)
{
    compile_loop(cc, while_stmt->condition, while_stmt->stmts, NULL);
}

static void compile_for(struct CompilerContext *cc, struct For *for_stmt)
{
    compile_statement(cc, for_stmt->init);
    compile_loop(cc, for_stmt->condition, for_stmt->stmts, for_stmt->increment);
}

static void compile_statement(struct CompilerContext *cc, struct Statement *stmt)
{
    switch (stmt->type) {
        case STMT_SET:     compile_set(cc,     stmt->set);         break;
        case STMT_RETURN:  compile_return(cc,  stmt->ret);         break;
        case STMT_IF:      compile_if(cc,      stmt->if_stmt);     break;
        case STMT_WHILE:   compile_while(cc,   stmt->while_stmt);  break;
        case STMT_FOR:     compile_for(cc,     stmt->for_stmt);    break;
        case STMT_FUNCALL: compile_funcall(cc, stmt->funcall);     break;
        case STMT_LET:     load_opcode(cc, DEFINE);                       break;
        default: printf("Error cannot compile statement type: not implemented\n"); break;
    }
}

static void compile_statements(struct CompilerContext *cc, Statements *stmts)
{
    linkedlist_foreach(lnode, stmts->head) {
        compile_statement(cc, lnode->value);
    }
}

// static int compile_let(struct CompilerContext *cc, Let *let, int offset)
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

// static void compile_class(struct CompilerContext *cc, struct Class *cls)
// {
//     compile_constructor(cc, cls);
// }

static void compile_entrypoint(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    linkedlist_foreach(lnode, tlds->head) {
        struct TopLevelDecl *tld = lnode->value;
        if (tld->type == TLD_TYPE_FUNDEF && tld->fundef->name == globals.entrypoint) {
            load_opcode(cc, PUSH_SCOPE);
            compile_statements(cc, tld->fundef->stmts);
            load_opcode(cc, POP_SCOPE);
            load_opcode(cc, EXIT);
            break;
        }
    }
}

static void compile_tld(struct CompilerContext *cc, struct TopLevelDecl *tld)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            // compile_class(cc, tld->cls);
            break;
        case TLD_TYPE_FUNDEF:
            if (tld->fundef->name != globals.entrypoint) {
                compile_fundef(cc, tld->fundef);
            }
            break;
    }
}

static void compile_tlds(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    cc->funcall_table = new_ft(0);
    compile_entrypoint(cc, tlds);
    linkedlist_foreach(lnode, tlds->head) {
        compile_tld(cc, (struct TopLevelDecl *) lnode->value);
    }
}

// Looks through compiled bytecode and adds references to where function is defined
static void resolve_function_declarations_help(DelValue *instructions,
    struct FunctionCallTableNode *fn)
{
    linkedlist_foreach(lnode, fn->callsites->head) {
        uint64_t *callsite = lnode->value;
        // printf("callsite %" PRIu64 " updated with function %s at location %" PRIu64,
        //         *callsite, lookup_symbol(fn->function), fn->location);
        instructions[*callsite].offset = fn->location;
    }
}

static void resolve_function_declarations(DelValue *instructions,
        struct FunctionCallTable *funcall_table)
{
    if (funcall_table == NULL) return;
    resolve_function_declarations_help(instructions, funcall_table->node);
    resolve_function_declarations(instructions, funcall_table->left);
    resolve_function_declarations(instructions, funcall_table->right);
    return;
}

// #include "test_compile.c"

size_t compile(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    compile_tlds(cc, tlds);
    resolve_function_declarations(cc->instructions, cc->funcall_table);
    return cc->offset;
    // run_tests();
    // printf("compiler under construction. come back later.\n");
    // exit(0);
}

