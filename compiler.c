#include "common.h"
#include "ast.h"
#include "compiler.h"

static void compile_value(struct CompilerContext *cc, struct Value *val);
static void compile_expr(struct CompilerContext *cc, struct Expr *expr);
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

static void compile_loadsym(struct CompilerContext *cc, Symbol symbol)
{
    load(cc, LOAD);
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

static void compile_funargs(struct CompilerContext *cc, struct FunDef *fundef)
{
    for (Definitions *defs = fundef->args; defs != NULL; defs = defs->next) {
        struct Definition *def = (struct Definition *) defs->value;
        load(cc, PUSH);
        load(cc, def->name);
        load(cc, DEF);
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
    add_function(cc->ft, fundef->name, cc->offset);
    compile_funargs(cc, fundef);
    compile_statements(cc, fundef->stmts);
}

static void compile_funcall(struct CompilerContext *cc, struct FunCall *funcall)
{
    load(cc, PUSH);
    int bookmark = next(cc);
    for (Values *args = funcall->args; args != NULL; args = args->next) {
        compile_value(cc, args->value);
    }
    load(cc, PUSH);
    add_callsite(cc->ft, funcall->funname, next(cc));
    load(cc, JMP);
    cc->instructions[bookmark] = cc->offset;
}

static void compile_value(struct CompilerContext *cc, struct Value *val)
{
    switch (val->type) {
        case VTYPE_STRING:  compile_string(cc, val->string);   break;
        case VTYPE_INT:     compile_int(cc, val->integer);     break;
        case VTYPE_SYMBOL:  compile_loadsym(cc, val->symbol);  break;
        case VTYPE_EXPR:    compile_expr(cc, val->expr);       break;
        case VTYPE_FUNCALL: compile_funcall(cc, val->funcall); break;
        default: printf("compile not implemented yet\n"); assert(0);
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
    load(cc, PUSH);
    load(cc, set->symbol);
    load(cc, DEF);
}

static void compile_return(struct CompilerContext *cc, struct Value *ret)
{
    compile_value(cc, ret);
    load(cc, SWAP);
    load(cc, JMP);
}

static void compile_if(struct CompilerContext *cc, struct IfStatement *stmt)
{
    int top_of_loop = 0;
    int old_offset = 0;
    top_of_loop = cc->offset;
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

static void compile_while(struct CompilerContext *cc, struct While *stmt)
{
    int top_of_loop, old_offset;
    top_of_loop = cc->offset;
    load(cc, PUSH);
    old_offset = next(cc);
    compile_value(cc, stmt->condition);
    load(cc, JNE);
    compile_statements(cc, stmt->stmts);
    load(cc, PUSH);
    load(cc, top_of_loop);
    load(cc, JMP);
    cc->instructions[old_offset] = (uint64_t) cc->offset; // set JNE jump to go to end of loop
}

static void compile_statement(struct CompilerContext *cc, struct Statement *stmt)
{
    switch (stmt->type) {
        case STMT_SET:    compile_set(cc, stmt->set);          break;
        case STMT_RETURN: compile_return(cc, stmt->ret);       break;
        case STMT_IF:     compile_if(cc, stmt->if_stmt);       break;
        case STMT_WHILE:  compile_while(cc, stmt->while_stmt); break;
        default: printf("Error cannot compile statement type: not implemented\n"); break;
    }
}

static void compile_statements(struct CompilerContext *cc, Statements *stmts)
{
    for (; stmts != NULL; stmts = stmts->next) {
        compile_statement(cc, stmts->value);
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
//         load(DEF);
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
//             load(DEF); // will work on an alternate "SET" operation later.
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

static int compile_class(struct CompilerContext *cc, struct Class *cls)
{
    return -1;
}

static void compile_entrypoint(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    for (; tlds != NULL; tlds = tlds->next) {
        struct TopLevelDecl *tld = (struct TopLevelDecl *) tlds->value;
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
        case TLD_TYPE_CLASS: compile_class(cc, tld->cls); break;
        case TLD_TYPE_FUNDEF:
            if (tld->fundef->name != ast.entrypoint) {
                compile_fundef(cc, tld->fundef);
            }
            break;
    }
}

static void compile_tlds(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    cc->ft = new_ft(0);
    compile_entrypoint(cc, tlds);
    for (;tlds != NULL; tlds = tlds->next) {
        compile_tld(cc, (struct TopLevelDecl *) tlds->value);
    }
}

// Looks through compiled bytecode and adds references to where function is defined
static void resolve_function_declarations_help(uint64_t *instructions, struct FunctionTableNode *fn)
{
    for (struct List *calls = fn->callsites; calls != NULL; calls = calls->prev) {
        uint64_t *callsite = (uint64_t *) calls->value;
        printf("callsite %llu updated with function %s at location %llu",
                *callsite, lookup_symbol(fn->function), fn->location);
        instructions[*callsite] = fn->location;
    }
}

static void resolve_function_declarations(uint64_t *instructions, struct FunctionTable *ft)
{
    if (ft == NULL) return;
    resolve_function_declarations_help(instructions, ft->node);
    resolve_function_declarations(instructions, ft->left);
    resolve_function_declarations(instructions, ft->right);
    return;
}

// #include "test_compile.c"

int compile(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    // compile_statements(cc, stmts, offset);
    // cc->instructions[offset] = (uint64_t) RET;
    compile_tlds(cc, tlds);
    resolve_function_declarations(cc->instructions, cc->ft);
    return cc->offset;
    // run_tests();
    // printf("compiler under construction. come back later.\n");
    // exit(0);
}

