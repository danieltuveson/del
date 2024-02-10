#include "common.h"
#include "ast.h"
#include "compiler.h"

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
static int compile_string(struct CompilerContext *cc, char *string)
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

static void compile_value(struct CompilerContext *cc, struct Value *val);
static void compile_expr(struct CompilerContext *cc, struct Expr *expr);

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

static void compile_value(struct CompilerContext *cc, struct Value *val)
{
    switch (val->type) {
        case VTYPE_STRING:
            compile_string(cc, val->string);
            break;
        case VTYPE_INT:
            compile_int(cc, val->integer);
            break;
        case VTYPE_SYMBOL:
            compile_loadsym(cc, val->symbol);
            break;
        case VTYPE_EXPR:
            compile_expr(cc, val->expr);
            break;
//         case VTYPE_FLOAT:
//         case VTYPE_BOOL:
//         // case VTYPE_OBJECT:
//             // Just using function call semantics to experiment with heap allocation
//             // offset = compile_value(cc, val->funcall->values->value, next());
//             // load(PUSH);
//             // load(1);
//             // load(PUSH_HEAP);
//             // load(val->funcall->funname);
//             // struct FunCall { Symbol funname; Values *values; };
//             assert("Error compiling unexpected value\n");
//             break;
//         case VTYPE_FUNCALL:
//             offset = compile_funcall(cc, val->funcall, offset);
//             break;
        default:
            printf("compile not implemented yet\n");
            assert(0);
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

// static int compile_funcall(struct CompilerContext *cc, struct FunCall *funcall, int offset)
// {
//     printf("offset = %d\n", offset);
//     load(PUSH);
//     int bookmark = next();
//     for (Values *args = funcall->args; args != NULL; args = args->next) {
//         offset = compile_value(cc, args->value, offset);
//     }
//     load(PUSH);
//     add_callsite(cc->ft, funcall->funname, next());
//     load(JMP);
//     cc->instructions[bookmark] = offset;
//     return offset;
// }
// 
// static int compile_if(struct CompilerContext *cc, struct Statement *stmt, int offset)
// {
//     int top_of_loop = 0;
//     int old_offset = 0;
//     top_of_loop = offset;
//     load(PUSH);
//     old_offset = next();
//     offset = compile_value(cc, stmt->if_stmt->condition, next());
//     load(JNE);
//     offset = compile_statements(cc, stmt->if_stmt->if_stmts, next());
// 
//     // set JNE jump to go to after if statement
//     cc->instructions[old_offset] = (uint64_t) offset;
// 
//     if (stmt->if_stmt->else_stmts) {
//         offset = compile_statements(cc, stmt->if_stmt->else_stmts, next());
//     }
//     return offset;
// }
// 
// static int compile_while(struct CompilerContext *cc, struct Statement *stmt, int offset)
// {
//     int top_of_loop = offset;
//     load(PUSH);
//     int old_offset = next();
//     offset = compile_value(cc, stmt->while_stmt->condition, next());
//     load(JNE);
//     offset = compile_statements(cc, stmt->while_stmt->stmts, next());
//     load(PUSH);
//     load(top_of_loop);
//     load(JMP);
//     cc->instructions[old_offset] = (uint64_t) offset; // set JNE jump to go to end of loop
//     return offset;
// }
// 
// static int compile_dim(struct CompilerContext *cc, Dim *dim, int offset)
// {
//     for (; dim != NULL; dim = dim->next) {
//         // TODO: check that value type is valid
//         struct Definition *def = (struct Definition *) dim->value;
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
//         case STMT_DIM:
//             offset = compile_dim(cc, stmt->dim, offset);
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
// static int compile_statements(struct CompilerContext *cc, Statements *stmts, int offset)
// {
//     for (; stmts != NULL; stmts = stmts->next) {
//         offset = compile_statement(cc, stmts->value, offset);
//     }
//     return offset;
// }
// 
// // static int compile_class(struct CompilerContext *cc, struct TopLevelDecl *tld, int offset)
// // {
// //     return -1;
// // }
// 
// static int compile_entrypoint(struct CompilerContext *cc, TopLevelDecls *tlds, int offset)
// {
//     struct TopLevelDecl *tld = NULL;
//     for (; tlds != NULL; tlds = tlds->next) {
//         tld = (struct TopLevelDecl *) tlds->value;
//         if (tld->type == TLD_TYPE_FUNDEF && tld->fundef->name == ast.entrypoint) {
//             offset = compile_statements(cc, tld->fundef->stmts, offset);
//             load(EXIT);
//             return offset;
//         }
//     }
//     return offset;
// }
// 
// /* Pop arguments from stack, use to define function args
//  * - Enter new scope
//  * - Execute statements
//  * - Push return value to stack
//  * - Exit new scope (pop off local variables defined in this scope)
//  * - Jump back to caller
//  */
// static int compile_fundef(struct CompilerContext *cc, struct FunDef *fundef, int offset)
// {
//     add_function(cc->ft, fundef->name, offset);
//     // Compile arguments
//     for (Definitions *defs = fundef->args; defs != NULL; defs = defs->prev) {
//         struct Definition *def = (struct Definition *) defs->value;
//         load(PUSH);
//         load(def->name);
//         // load(PUSH);
//         // load(0); // Initialize to 0
//         load(DEF);
//     }
//     // Compile statements
//     offset = compile_statements(cc, fundef->stmts, offset);
// 
//     return offset;
// }
// 
// static int compile_tlds(struct CompilerContext *cc, TopLevelDecls *tlds)
// {
//     struct TopLevelDecl *tld = NULL;
//     cc->ft = new_ft(0);
//     int offset = compile_entrypoint(cc, tlds, 0);
//     for (;tlds != NULL; tlds = tlds->next) {
//         tld = (struct TopLevelDecl *) tlds->value;
//         switch (tld->type) {
//             case TLD_TYPE_CLASS:
//                 offset = compile_fundef(cc, tld->fundef, offset);
//                 break;
//             case TLD_TYPE_FUNDEF:
//                 if (tld->fundef->name != ast.entrypoint) {
//                     offset = compile_fundef(cc, tld->fundef, offset);
//                 }
//                 break;
//         }
//     }
//     return offset;
// }
// 
// // Looks through compiled bytecode and adds references to where function is defined
// static void resolve_function_declarations_help(uint64_t *instructions, struct FunctionTableNode *fn)
// {
//     for (struct List *calls = fn->callsites; calls != NULL; calls = calls->prev) {
//         uint64_t *callsite = (uint64_t *) calls->value;
//         printf("callsite %llu updated with function %s at location %llu",
//                 *callsite, lookup_symbol(fn->function), fn->location);
//         instructions[*callsite] = fn->location;
//     }
// }
// 
// static void resolve_function_declarations(uint64_t *instructions, struct FunctionTable *ft)
// {
//     if (ft == NULL) return;
//     resolve_function_declarations_help(instructions, ft->node);
//     resolve_function_declarations(instructions, ft->left);
//     resolve_function_declarations(instructions, ft->right);
//     return;
// }
// 

#include "test_compile.c"

int compile(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    // offset = compile_statements(cc, stmts, offset);
    // cc->instructions[offset] = (uint64_t) RET;
    // int offset = compile_tlds(cc, tlds);
    // resolve_function_declarations(cc->instructions, cc->ft);
    // return offset;
    run_tests();
    printf("compiler under construction. come back later.\n");
    exit(0);
}

