#include "common.h"
#include "compiler.h"

static inline int next_safe(int offset) {
    assert("offset is out of bounds\n" && offset < INSTRUCTIONS_SIZE - 1);
    return offset;
}
#define next() next_safe(offset++)

/* The load macro assumes that the variables "instructions" and "offset" are
 * passed into the functions in which it is called.
 */
#define load(op) instructions[next()] = (uint64_t) op
#define compile_binary_op(instructions, val1, val2, offset, op)\
    compile_binary_op_helper(instructions, val1, val2, &offset, (uint64_t) op)

static int compile_value(uint64_t *instructions, struct Value *value, int offset);
static int compile_statements(uint64_t *instructions, Statements *stmts, int offset);

static inline void compile_binary_op_helper(uint64_t *instructions,
        struct Value *val1, struct Value *val2, int *offset_ptr, uint64_t op) {
    int offset = *offset_ptr;
    offset = compile_value(instructions, val1, next());
    offset = compile_value(instructions, val2, next());
    load(op);
    *offset_ptr = offset;
}

/* Boring and takes up too much space.
 * Scrunching it up until I can think of something better!
 */
static int compile_expr(uint64_t *instructions, struct Expr *expr, int offset)
{
    struct Value *val1, *val2;
    val1 = expr->val1;
    val2 = expr->val2;
    switch (expr->op) {
        case OP_OR: compile_binary_op(instructions, val1, val2, offset, OR); break;
        case OP_AND: compile_binary_op(instructions, val1, val2, offset, AND); break;
        case OP_EQEQ: compile_binary_op(instructions, val1, val2, offset, EQ); break;
        case OP_NOT_EQ: compile_binary_op(instructions, val1, val2, offset, NEQ); break;
        case OP_GREATER_EQ: compile_binary_op(instructions, val1, val2, offset, GTE); break;
        case OP_GREATER: compile_binary_op(instructions, val1, val2, offset, GT); break;
        case OP_LESS_EQ: compile_binary_op(instructions, val1, val2, offset, LTE); break;
        case OP_LESS: compile_binary_op(instructions, val1, val2, offset, LT); break;
        case OP_PLUS: compile_binary_op(instructions, val1, val2, offset, ADD); break;
        case OP_MINUS: compile_binary_op(instructions, val1, val2, offset, SUB); break;
        case OP_STAR: compile_binary_op(instructions, val1, val2, offset, MUL); break;
        case OP_SLASH: compile_binary_op(instructions, val1, val2, offset, DIV); break;
        case OP_UNARY_PLUS:
            offset = compile_value(instructions, val1, next());
            load(UNARY_PLUS);
            break;
        case OP_UNARY_MINUS:
            offset = compile_value(instructions, val1, next());
            load(UNARY_MINUS);
            break;
        default:
            printf("Error cannot compile expression\n");
            break;
    }
    return offset;
}

// Pack 8 byte chunks of chars into longs to push onto stack
static int compile_string(uint64_t *instructions, char *string, int offset)
{
    uint64_t packed = 0;
    uint64_t i = 0;
    uint64_t tmp;
    for (; string[i] != '\0'; i++) {
        tmp = (uint64_t) string[i];
        switch ((i + 1) % 8) {
            case 0:
                packed = packed | (tmp << 56);
                load(PUSH);
                load(packed);
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
        load(PUSH);
        load(packed);
        // push count
        load(PUSH);
        load(i / 8 + 1);
    } else {
        load(PUSH);
        load(i / 8);
    }
    load(PUSH_HEAP);
    return offset;
    // offset = compile_value(instructions, val->funcall->values->value, next());
    // load(PUSH);
    // load(1);
    // load(PUSH_HEAP);
}

static int compile_funcall(uint64_t *instructions, struct FunCall *funcall, int offset)
{
    load(PUSH);
    int bookmark = next();
    for (Values *args = funcall->args; args != NULL; args = args->next) {
        offset = compile_value(instructions, args->value, next());
    }
    load(JMP);
    add_callsite(ft, funcall->funname, next());
    instructions[bookmark] = offset;
    return offset;
}

static int compile_value(uint64_t *instructions, struct Value *val, int offset)
{
    switch (val->type) {
        case VTYPE_STRING:
            offset = compile_string(instructions, val->string, offset);
            break;
        case VTYPE_FLOAT:
        case VTYPE_BOOL:
        // case VTYPE_OBJECT:
            // Just using function call semantics to experiment with heap allocation
            // offset = compile_value(instructions, val->funcall->values->value, next());
            // load(PUSH);
            // load(1);
            // load(PUSH_HEAP);
            // load(val->funcall->funname);
            // struct FunCall { Symbol funname; Values *values; };
            assert("Error compiling unexpected value\n");
            break;
        case VTYPE_FUNCALL:
            offset = compile_funcall(instructions, val->funcall, offset);
            break;
        case VTYPE_INT:
            load(PUSH);
            load(val->integer);
            break;
        case VTYPE_SYMBOL:
            load(LOAD);
            load(val->symbol);
            // offset = compile_string(instructions, val->symbol, offset);
            break;
        case VTYPE_EXPR:
            offset = compile_expr(instructions, val->expr, offset);
            break;
    }
    return offset;
}

static int compile_if(uint64_t *instructions, struct Statement *stmt, int offset)
{
    int top_of_loop = 0;
    int old_offset = 0;
    top_of_loop = offset;
    load(PUSH);
    old_offset = next();
    offset = compile_value(instructions, stmt->if_stmt->condition, next());
    load(JNE);
    offset = compile_statements(instructions, stmt->if_stmt->if_stmts, next());

    // set JNE jump to go to after if statement
    instructions[old_offset] = (uint64_t) offset;

    if (stmt->if_stmt->else_stmts) {
        offset = compile_statements(instructions, stmt->if_stmt->else_stmts, next());
    }
    return offset;
}

static int compile_while(uint64_t *instructions, struct Statement *stmt, int offset)
{
    int top_of_loop = offset;
    load(PUSH);
    int old_offset = next();
    offset = compile_value(instructions, stmt->while_stmt->condition, next());
    load(JNE);
    offset = compile_statements(instructions, stmt->while_stmt->stmts, next());
    load(PUSH);
    load(top_of_loop);
    load(JMP);
    instructions[old_offset] = (uint64_t) offset; // set JNE jump to go to end of loop
    return offset;
}

static int compile_dim(uint64_t *instructions, Dim *dim, int offset)
{
    for (; dim != NULL; dim = dim->next) {
        // TODO: check that value type is valid
        struct Definition *def = (struct Definition *) dim->value;
        load(PUSH);
        load(def->name);
        // offset = compile_string(instructions, def->name, offset);
        load(PUSH);
        load(0); // Initialize to 0
        load(DEF);
    }
    return offset;
}

static int compile_statement(uint64_t *instructions, struct Statement *stmt, int offset)
{
    switch (stmt->type) {
        case STMT_SET:
            load(PUSH);
            load(stmt->set->symbol);
            offset = compile_value(instructions, stmt->set->val, next());
            load(DEF); // will work on an alternate "SET" operation later.
            break;
        case STMT_IF:
            offset = compile_if(instructions, stmt, offset);
            break;
        case STMT_WHILE:
            offset = compile_while(instructions, stmt, offset);
            break;
        case STMT_DIM:
            offset = compile_dim(instructions, stmt->dim, offset);
            break;
        case STMT_FUNCALL:
            // offset = offset; // no op
            // load(PUSH);
            // int old_offset = next();
            // long count = 0;
            // Values *values = stmt->funcall->values;
            // while (values != NULL) {
            //     struct Value *val = (struct Value *) values->value;
            //     offset = compile_value(instructions, val, offset);
            //     values = values->next;
            //     count++;
            // }
            // load(PUSH);
            // load(stmt->funcall->funname);
            // // offset = compile_string(instructions, stmt->funcall->funname, offset);
            // load(CALL);
            // load(POP); // clear returned value off of the stack since this is a statement
            // instructions[old_offset] = (uint64_t) count; // pass in number of arguments
            // // struct FunCall { Symbol funname; Values *values; };
            // break;
        case STMT_FUNCTION_DEF:
        case STMT_EXIT_FOR:
        case STMT_EXIT_WHILE:
        case STMT_EXIT_FUNCTION:
        case STMT_FOR:
            // stmt->for_stmt->start;
            // stmt->for_stmt->stop;
            // stmt->for_stmt->stmts;
        case STMT_FOREACH:
            // stmt->for_each->symbol;
            // stmt->for_each->condition;
            // stmt->for_each->stmts;
        default:
            printf("cannot compile statement type: not implemented\n");
            assert(0);
            break;
    }
    return offset;
}

static int compile_statements(uint64_t *instructions, Statements *stmts, int offset)
{
    for (; stmts != NULL; stmts = stmts->next) {
        offset = compile_statement(instructions, stmts->value, offset);
    }
    return offset;
}

// static int compile_class(uint64_t *instructions, struct TopLevelDecl *tld, int offset)
// {
//     return -1;
// }
// 
// static int compile_fundef(uint64_t *instructions, struct FunDef *fundef, int offset)
// {
// }

static int compile_entrypoint(uint64_t *instructions, TopLevelDecls *tlds, int offset)
{
    struct TopLevelDecl *tld = NULL;
    for (; tlds != NULL; tlds = tlds->next) {
        tld = (struct TopLevelDecl *) tlds->value;
        if (tld->type == TLD_TYPE_FUNDEF && tld->fundef->name == ast.entrypoint) {
            offset = compile_statements(instructions, tld->fundef->stmts, offset);
            instructions[offset] = (uint64_t) RET;
            return offset;
        }
    }
    return offset;
}

static int compile_fundef(uint64_t *instructions, struct FunDef *fundef, int offset)
{
    // New scope
    //
    // Return value
    // Exit scope
    // Pop off local variables declared in this scope
    return offset;
}

static int compile_tlds(uint64_t *instructions, TopLevelDecls *tlds, int offset)
{
    struct TopLevelDecl *tld = NULL;
    struct FunctionTable *ft = new_ft(0);
    offset = compile_entrypoint(instructions, tlds, offset);
    for (;tlds != NULL; tlds = tlds->next) {
        tld = (struct TopLevelDecl *) tlds->value;
        switch (tld->type) {
            case TLD_TYPE_CLASS:
                offset = compile_fundef(instructions, tld->fundef, offset);
                break;
            case TLD_TYPE_FUNDEF:
                if (tld->fundef->name == ast.entrypoint) {
                    break;
                }
                offset = compile_fundef(instructions, tld->fundef, offset);
                break;
        }
    }
    return offset;
}

int compile(struct CompilerContext *cc, TopLevelDecls *tlds)
{
    // offset = compile_statements(instructions, stmts, offset);
    // instructions[offset] = (uint64_t) RET;
    cc->offset = compile_tlds(cc->instructions, tlds, cc->offset);
    return cc->offset;
}
 
#undef next
#undef load
#undef compile_binary_op

