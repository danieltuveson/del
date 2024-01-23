#include "common.h"
#include "compiler.h"

#define load(op) instructions[offset++] = (void *) op

static int compile_value(void **instructions, struct Value *value, int offset);

static inline void compile_binary_op_helper(void **instructions, struct Value *val1, struct Value *val2, int *offset_ptr, void *op) {
    int offset = *offset_ptr;
    offset = compile_value(instructions, val1, offset++);
    offset = compile_value(instructions, val2, offset++);
    load(op);
    *offset_ptr = offset;
}
#define compile_binary_op(instructions, val1, val2, offset, op) compile_binary_op_helper(instructions, val1, val2, &offset, (void *) op)

static int compile_value(void **instructions, struct Value *val, int offset)
{
    struct Value *val1, *val2;
    switch (val->type) {
        case VTYPE_STRING:
        case VTYPE_FLOAT:
        case VTYPE_BOOL:
        case VTYPE_FUNCALL:
            // struct FunCall { Symbol funname; Values *values; };
            assert("Error compiling unexpected value\n");
            break;
        case VTYPE_INT:
            load(PUSH);
            load(val->integer);
            break;
        case VTYPE_SYMBOL:
            load(LOAD);
            load(val->symbol);
            break;
        case VTYPE_EXPR:
            val1 = val->expr->val1;
            val2 = val->expr->val2;
            switch (val->expr->op) {
                case OP_OR:
                    compile_binary_op(instructions, val1, val2, offset, OR);
                    break;
                case OP_AND:
                    compile_binary_op(instructions, val1, val2, offset, AND);
                    break;
                case OP_EQEQ:
                    compile_binary_op(instructions, val1, val2, offset, EQ);
                    break;
                case OP_NOT_EQ:
                    compile_binary_op(instructions, val1, val2, offset, NEQ);
                    break;
                case OP_GREATER_EQ:
                    compile_binary_op(instructions, val1, val2, offset, GTE);
                    break;
                case OP_GREATER:
                    compile_binary_op(instructions, val1, val2, offset, GT);
                    break;
                case OP_LESS_EQ:
                    compile_binary_op(instructions, val1, val2, offset, LTE);
                    break;
                case OP_LESS:
                    compile_binary_op(instructions, val1, val2, offset, LT);
                    break;
                case OP_PLUS:
                    compile_binary_op(instructions, val1, val2, offset, ADD);
                    break;
                case OP_MINUS:
                    compile_binary_op(instructions, val1, val2, offset, SUB);
                    break;
                case OP_STAR:
                    compile_binary_op(instructions, val1, val2, offset, MUL);
                    break;
                case OP_SLASH:
                    compile_binary_op(instructions, val1, val2, offset, DIV);
                    break;
                case OP_UNARY_PLUS:
                    offset = compile_value(instructions, val1, offset++);
                    load(UNARY_PLUS);
                    break;
                case OP_UNARY_MINUS:
                    offset = compile_value(instructions, val1, offset++);
                    load(UNARY_MINUS);
                    break;
                default:
                    printf("Error cannot compile expression\n");
                    break;
            }
            break;
    }
    return offset;
}

static int compile_statements(void **instructions, Statements *stmts, int offset);

static int compile_if(void **instructions, struct Statement *stmt, int offset)
{
    int top_of_loop = 0;
    int old_offset = 0;
    top_of_loop = offset;
    load(PUSH);
    old_offset = offset++;
    offset = compile_value(instructions, stmt->if_stmt->condition, offset++);
    load(JNE);
    offset = compile_statements(instructions, stmt->if_stmt->if_stmts, offset++);

    // set JNE jump to go to after if statement
    instructions[old_offset] = (void *) offset;

    if (stmt->if_stmt->else_stmts) {
        offset = compile_statements(instructions, stmt->if_stmt->else_stmts, offset++);
    }
    return offset;
}

static int compile_while(void **instructions, struct Statement *stmt, int offset)
{
    int top_of_loop = offset;
    load(PUSH);
    int old_offset = offset++;
    offset = compile_value(instructions, stmt->while_stmt->condition, offset++);
    load(JNE);
    offset = compile_statements(instructions, stmt->while_stmt->stmts, offset++);
    load(PUSH);
    load(top_of_loop);
    load(JMP);
    instructions[old_offset] = (void *) offset; // set JNE jump to go to end of loop
    return offset;
}

static int compile_dim(void **instructions, Dim *dim, int offset)
{
    while (dim != NULL) {
        // TODO: check that value type is valid
        struct Definition *def = (struct Definition *) dim->value;
        load(PUSH);
        load(def->name);
        load(PUSH);
        load(0); // Initialize to 0
        load(DEF);
        dim = dim->next;
    }
    return offset;
}

static int compile_statement(void **instructions, struct Statement *stmt, int offset)
{
    switch (stmt->type) {
        case STMT_SET:
            load(PUSH);
            load(stmt->set->symbol);
            offset = compile_value(instructions, stmt->set->val, offset++);
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
        case STMT_FOR:
            // stmt->for_stmt->start;
            // stmt->for_stmt->stop;
            // stmt->for_stmt->stmts;
        case STMT_FOREACH:
            // stmt->for_each->symbol;
            // stmt->for_each->condition;
            // stmt->for_each->stmts;
        case STMT_FUNCALL:
            offset = offset; // no op
            int old_offset = offset++;
            long count = 0;
            Values *values = stmt->funcall->values;
            while (values != NULL) {
                struct Value *val = (struct Value *) values->value;
                offset = compile_value(instructions, val, offset);
                values = values->next;
                count++;
            }
            load(PUSH);
            load(stmt->funcall->funname);
            load(CALL);
            load(POP); // clear returned value off of the stack since this is a statement
            instructions[old_offset] = (void *) count; // pass in number of arguments
            // struct FunCall { Symbol funname; Values *values; };
            break;
        case STMT_FUNCTION_DEF:
        case STMT_EXIT_FOR:
        case STMT_EXIT_WHILE:
        case STMT_EXIT_FUNCTION:
        default:
            printf("cannot compile statement type: not implemented\n");
            assert(0);
            break;
    }
    return offset;
}

static int compile_statements(void **instructions, Statements *stmts, int offset)
{
    while (stmts != NULL) {
        offset = compile_statement(instructions, stmts->value, offset);
        stmts = stmts->next;
    }
    // offset = compile_statement(instructions, stmts->value, offset);
    // if (stmts->next == NULL) {
    //     return offset;
    // } else {
    //     return compile_statements(instructions, stmts->next, offset);
    // }
}

int compile(void **instructions, Statements *stmts, int offset)
{
    // struct Expr *expr;
    // do {
    //     expr = exprs->expr;
    //     offset = compile_expression(instructions, expr, offset);
    //     // Discard top level expression from the stack since it's not part of a subexpression
    //     // (unless it's a statement that returns nothing)
    //     if (expr->type == EXPRESSION && (expr->binexpr->op != WHILE && expr->binexpr->op != IF)) {
    //         load(POP);
    //     }
    //     instructions[offset] = (void *) RET;
    //     exprs = exprs->next;
    // } while (exprs != NULL);

    offset = compile_statements(instructions, stmts, offset);
    instructions[offset] = (void *) RET;
    return offset;
}

