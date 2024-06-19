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
static inline int next(struct CompilerContext *cc) {
    assert("offset is out of bounds\n" && cc->offset < INSTRUCTIONS_SIZE - 1);
    return cc->offset++;
}

/* Add instruction to instruction set */
static inline void load(struct CompilerContext *cc, uint64_t op)
{
    cc->instructions[next(cc)] = op;
}

static void compile_int(struct CompilerContext *cc, uint64_t integer)
{
    load(cc, PUSH);
    load(cc, integer);
}

static void compile_float(struct CompilerContext *cc, double floating)
{
    assert("floating point not implemented\n" && 0);
    load(cc, PUSH);
    load(cc, floating);
}

static inline void compile_bool(struct CompilerContext *cc, uint64_t boolean)
{
    return compile_int(cc, boolean);
}

static void compile_loadsym(struct CompilerContext *cc, Symbol symbol)
{
    load(cc, GET_LOCAL);
    load(cc, symbol);
}

// Pack 8 byte chunks of chars into longs to push onto stack
static void compile_string(struct CompilerContext *cc, char *string)
{
    uint64_t packed = 0;
    uint64_t i = 0;
    uint64_t tmp;
    for (; string[i] != '\0'; i++) {
        tmp = (uint64_t) string[i];
        switch ((i + 1) % 8) {
            case 0:
                packed = packed | (tmp << 56);
                load(cc, PUSH);
                load(cc, packed);
                break;
            case 1: packed = tmp    | (tmp << 0);  break; /* technically "tmp << 0" does nothing */
            case 2: packed = packed | (tmp << 8);  break;
            case 3: packed = packed | (tmp << 16); break;
            case 4: packed = packed | (tmp << 24); break;
            case 5: packed = packed | (tmp << 32); break;
            case 6: packed = packed | (tmp << 40); break;
            case 7: packed = packed | (tmp << 48); break;
        }
    }
    // If string is not multiple of 8 bytes, push the remainder
    if ((i + 1) % 8 != 0) {
        load(cc, PUSH);
        load(cc, packed);
        // push count
        load(cc, PUSH);
        load(cc, i / 8 + 1);
    } else {
        load(cc, PUSH);
        load(cc, i / 8);
    }
    load(cc, PUSH_HEAP);
}

static void compile_binary_op(struct CompilerContext *cc, struct Value *val1, struct Value *val2,
       enum Code op) {
    compile_value(cc, val1);
    compile_value(cc, val2);
    load(cc, op);
}

static void compile_unary_op(struct CompilerContext *cc, struct Value *val, enum Code op) {
    compile_value(cc, val);
    load(cc, op);
}

static void compile_funargs(struct CompilerContext *cc, Definitions *defs)
{
    linkedlist_foreach(lnode, defs->head) {
    // for (; defs != NULL; defs = defs->next) {
        struct Definition *def = lnode->value;
        load(cc, PUSH);
        load(cc, def->name);
        load(cc, SET_LOCAL);
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
    // for (Values *args = seek_end(funcall->args); args != NULL; args = args->prev) {
        // printf("...compiling constructor...\n");
        // print_value(args->value);
        // printf("\n");
        compile_value(cc, lnode->value);
    }
    load(cc, PUSH);
    load(cc, funcall->args->length);
    load(cc, PUSH_HEAP);
}

static void compile_get(struct CompilerContext *cc, struct Accessor *get)
{
    if (get->lvalues == NULL) {
        load(cc, PUSH);
        load(cc, get->symbol);
        load(cc, GET_LOCAL);
        return;
    } 
    compile_loadsym(cc, get->symbol);
    Type parent_value_type = get->type;
    linkedlist_foreach(lnode, get->lvalues->head) {
    // for (LValues *lvalues = get->lvalues; lvalues != NULL; lvalues = lvalues->next) {
        struct Class *cls = lookup_class(cc->class_table, parent_value_type);
        struct LValue *lvalue = lnode->value;
        switch (lvalue->lvtype) {
            case LV_PROPERTY: {
                uint64_t index = lookup_property_index(cls, lvalue->property);
                load(cc, PUSH);
                load(cc, index);
                load(cc, GET_HEAP);
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
    load(cc, PUSH);
    int bookmark = next(cc);
    linkedlist_foreach_reverse(lnode, funcall->args->tail) {
    // for (Values *args = seek_end(funcall->args); args != NULL; args = args->prev) {
        compile_value(cc, lnode->value);
    }
    load(cc, PUSH);
    add_callsite(cc->funcall_table, funcall->funname, next(cc));
    load(cc, JMP);
    cc->instructions[bookmark] = cc->offset;
}

static void compile_value(struct CompilerContext *cc, struct Value *val)
{
    switch (val->vtype) {
        case VTYPE_STRING:      compile_string(cc,      val->string);   break;
        case VTYPE_INT:         compile_int(cc,         val->integer);  break;
        case VTYPE_FLOAT:       compile_float(cc,       val->floating); break;
        case VTYPE_BOOL:        compile_bool(cc,        val->boolean);  break;
        case VTYPE_SYMBOL:      compile_loadsym(cc,     val->symbol);   break;
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
        case OP_UNARY_PLUS:  compile_unary_op(cc, val1, UNARY_PLUS);  break;
        case OP_UNARY_MINUS: compile_unary_op(cc, val1, UNARY_MINUS); break;
        default: printf("Error cannot compile expression\n"); break;
    }
}

static void compile_set(struct CompilerContext *cc, struct Set *set)
{
    compile_value(cc, set->val);
    if (set->to_set->lvalues == NULL) {
        load(cc, PUSH);
        load(cc, set->to_set->symbol);
        load(cc, SET_LOCAL);
        return;
    } 
    // else {
    //     printf("Error cannot compile property access / index\n");
    //     return;
    // }
    compile_loadsym(cc, set->to_set->symbol);
    Type parent_value_type = set->to_set->type;
    linkedlist_foreach(lnode, set->to_set->lvalues->head) {
    // for (LValues *lvalues = set->to_set->lvalues; lvalues != NULL; lvalues = lvalues->next) {
        struct Class *cls = lookup_class(cc->class_table, parent_value_type);
        struct LValue *lvalue = lnode->value;
        switch (lvalue->lvtype) {
            case LV_PROPERTY: {
                uint64_t index = lookup_property_index(cls, lvalue->property);
                // NOTE: refactor to not have lookup_property on class, write a generic list lookup function
                if (lnode->next == NULL) {
                    load(cc, PUSH);
                    load(cc, index);
                    load(cc, SET_HEAP);
                } else {
                    load(cc, PUSH);
                    load(cc, index);
                    load(cc, GET_HEAP);
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
    compile_value(cc, ret);
    load(cc, SWAP);
    load(cc, JMP);
}

static void compile_if(struct CompilerContext *cc, struct IfStatement *stmt)
{
    int old_offset = 0;
    load(cc, PUSH);
    old_offset = next(cc);
    compile_value(cc, stmt->condition);
    load(cc, JNE);
    compile_statements(cc, stmt->if_stmts);

    // set JNE jump to go to after if statement
    cc->instructions[old_offset] = (uint64_t) cc->offset;

    if (stmt->else_stmts) {
        compile_statements(cc, stmt->else_stmts);
    }
}

static void compile_loop(struct CompilerContext *cc, struct Value *cond,
        Statements *stmts, struct Statement *increment)
{
    int top_of_loop, old_offset;
    top_of_loop = cc->offset;
    load(cc, PUSH);
    old_offset = next(cc);
    compile_value(cc, cond);
    load(cc, JNE);
    compile_statements(cc, stmts);
    if (increment != NULL) compile_statement(cc, increment);
    load(cc, PUSH);
    load(cc, top_of_loop);
    load(cc, JMP);
    cc->instructions[old_offset] = (uint64_t) cc->offset; // set JNE jump to go to end of loop
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
        case STMT_SET:    compile_set(cc,    stmt->set);          break;
        case STMT_RETURN: compile_return(cc, stmt->ret);          break;
        case STMT_IF:     compile_if(cc,     stmt->if_stmt);      break;
        case STMT_WHILE:  compile_while(cc,  stmt->while_stmt);   break;
        case STMT_FOR:    compile_for(cc,    stmt->for_stmt);     break;
        case STMT_LET: /* Currently just used for typechecking */ break;
        default: printf("Error cannot compile statement type: not implemented\n"); break;
    }
}

static void compile_statements(struct CompilerContext *cc, Statements *stmts)
{
    linkedlist_foreach(lnode, stmts->head) {
    // for (; stmts != NULL; stmts = stmts->next) {
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
// 
// static int compile_statement(struct CompilerContext *cc, struct Statement *stmt, int offset)
// {
//     switch (stmt->type) {
//         case STMT_SET:
//             load(PUSH);
//             offset = compile_value(cc, stmt->set->val, next());
//             load(stmt->set->symbol);
//             load(SET_LOCAL); // will work on an alternate "SET" operation later.
//             break;
//         case STMT_IF:
//             offset = compile_if(cc, stmt, offset);
//             break;
//         case STMT_WHILE:
//             offset = compile_while(cc, stmt, offset);
//             break;
//         case STMT_LET:
//             offset = compile_let(cc, stmt->let, offset);
//             break;
//         case STMT_RETURN:
//             offset = compile_value(cc, stmt->ret, offset);
//             load(SWAP);
//             load(JMP);
//             break;
//         case STMT_FUNCALL:
//         case STMT_FUNCTION_DEF:
//         case STMT_EXIT_FOR:
//         case STMT_EXIT_WHILE:
//         case STMT_EXIT_FUNCTION:
//         case STMT_FOR:
//             // stmt->for_stmt->start;
//             // stmt->for_stmt->stop;
//             // stmt->for_stmt->stmts;
//         case STMT_FOREACH:
//             // stmt->for_each->symbol;
//             // stmt->for_each->condition;
//             // stmt->for_each->stmts;
//         default:
//             printf("cannot compile statement type: not implemented\n");
//             assert(0);
//             break;
//     }
//     return offset;
// }
// 

// static void compile_class(struct CompilerContext *cc, struct Class *cls)
// {
//     compile_constructor(cc, cls);
// }

static void compile_entrypoint(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    linkedlist_foreach(lnode, tlds->head) {
    // for (; tlds != NULL; tlds = tlds->next) {
        struct TopLevelDecl *tld = lnode->value;
        if (tld->type == TLD_TYPE_FUNDEF && tld->fundef->name == ast.entrypoint) {
            compile_statements(cc, tld->fundef->stmts);
            load(cc, EXIT);
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
            if (tld->fundef->name != ast.entrypoint) {
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
    // for (;tlds != NULL; tlds = tlds->next) {
        compile_tld(cc, (struct TopLevelDecl *) lnode->value);
    }
}

// Looks through compiled bytecode and adds references to where function is defined
static void resolve_function_declarations_help(uint64_t *instructions, struct FunctionCallTableNode *fn)
{
    linkedlist_foreach(lnode, fn->callsites->head) {
    // for (struct List *calls = fn->callsites; calls != NULL; calls = calls->prev) {
        uint64_t *callsite = lnode->value;
        printf("callsite %" PRIu64 " updated with function %s at location %" PRIu64,
                *callsite, lookup_symbol(fn->function), fn->location);
        instructions[*callsite] = fn->location;
    }
}

static void resolve_function_declarations(uint64_t *instructions,
        struct FunctionCallTable *funcall_table)
{
    if (funcall_table == NULL) return;
    resolve_function_declarations_help(instructions, funcall_table->node);
    resolve_function_declarations(instructions, funcall_table->left);
    resolve_function_declarations(instructions, funcall_table->right);
    return;
}

// #include "test_compile.c"

int compile(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    compile_tlds(cc, tlds);
    resolve_function_declarations(cc->instructions, cc->funcall_table);
    return cc->offset;
    // run_tests();
    // printf("compiler under construction. come back later.\n");
    // exit(0);
}

