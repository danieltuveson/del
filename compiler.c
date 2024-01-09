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
            assert("Error compiling unexpected value\n");
            break;
        case VTYPE_INT:
            printf("got an integer\n");
            load(PUSH);
            load(val->integer);
            break;
        case VTYPE_SYMBOL:
            load(LOAD);
            load(val->symbol);
            break;
        case VTYPE_EXPR:
            val1 = val->expr->val1;
            val2 = val->expr->val1;
            switch (val->expr->op) {
                case OP_OR:
                    assert("Error compiling unexpected operator\n");
                    break;
                case OP_AND:
                    break;
                case OP_EQEQ:
                    compile_binary_op(instructions, val1, val2, offset, EQ);
                    break;
                case OP_NOT_EQ:
                    break;
                case OP_GREATER_EQ:
                    break;
                case OP_GREATER:
                    compile_binary_op(instructions, val1, val2, offset, GT);
                    break;
                case OP_LESS_EQ:
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
                    break;
                case OP_UNARY_MINUS:
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

static int compile_statement(void **instructions, struct Statement *stmt, int offset)
{
    int top_of_loop = 0;
    int old_offset = 0;
    Dim *dim = NULL;
    struct Definition *def = NULL;
    switch (stmt->type) {
        case STMT_SET:
            // stmt->set->symbol;
            // stmt->set->val;
            break;
        case STMT_IF:
            // stmt->if_stmt->else_stmts
            top_of_loop = offset;
            load(PUSH);
            old_offset = offset++;
            offset = compile_value(instructions, stmt->if_stmt->condition, offset++);
            load(JNE);
            offset = compile_statements(instructions, stmt->if_stmt->if_stmts, offset++);

            // set JNE jump to go to after if statement
            instructions[old_offset] = (void *) offset;
            break;
        case STMT_WHILE:
            top_of_loop = offset;
            load(PUSH);
            old_offset = offset++;
            offset = compile_value(instructions, stmt->while_stmt->condition, offset++);
            load(JNE);
            offset = compile_statements(instructions, stmt->while_stmt->stmts, offset++);
            load(PUSH);
            load(top_of_loop);
            load(JMP);
            instructions[old_offset] = (void *) offset; // set JNE jump to go to end of loop
            break;
        case STMT_DIM:
            dim = stmt->dim;
            while (dim != NULL) {
                // TODO: check that value type is valid
                def = (struct Definition *) stmt->dim->value;
                load(PUSH);
                load(def->name);
                // Initialize to 0
                load(PUSH);
                load(0);
                load(DEF);
                dim = dim->next;
            }
            break;
        case STMT_FOR:
            // stmt->for_stmt->start;
            // stmt->for_stmt->stop;
            // stmt->for_stmt->stmts;
        case STMT_FOREACH:
            // stmt->for_each->symbol;
            // stmt->for_each->condition;
            // stmt->for_each->stmts;
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
    offset = compile_statement(instructions, stmts->value, offset);
    if (stmts->next == NULL) {
        return offset;
    } else {
        return compile_statements(instructions, stmts->next, offset);
    }
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

