#include "common.h"
#include "printers.h"


void print_instructions(uint64_t *instructions, int length)
{
    for (int i = 0; i < length; i++) {
        printf("%-5d", i);
        switch ((enum Code) instructions[i]) {
            case PUSH:
                i++;
                printf("PUSH %li\n", (long) instructions[i]);
                break;
            case ADD:
                printf("ADD\n");
                break;
            case SUB:
                printf("SUB\n");
                break;
            case MUL:
                printf("MUL\n");
                break;
            case DIV:
                printf("DIV\n");
                break;
            case JE:
                printf("JE\n");
                break;
            case JMP:
                printf("JMP\n");
                break;
            case JNE:
                printf("JNE\n");
                break;
            case RET:
                printf("RET\n");
                break;
            case DEF:
                printf("DEF\n");
                break;
            case POP:
                printf("POP\n");
                break;
            case EQ:
                printf("EQ\n");
                break;
            case NEQ:
                printf("NEQ\n");
                break;
            case LT:
                printf("LT\n");
                break;
            case GT:
                printf("GT\n");
                break;
            case AND:
                printf("AND\n");
                break;
            case OR:
                printf("OR\n");
                break;
            case LTE:
                printf("LTE\n");
                break;
            case GTE: 
                printf("GTE\n");
                break;
            case UNARY_PLUS: 
                printf("UNARY_PLUS\n");
                break;
            case UNARY_MINUS:
                printf("UNARY_MINUS\n");
                break;
            case SET:
                printf("SET\n");
                break;
            case CALL:
                printf("CALL\n");
                break;
            case LOAD:
                i++;
                printf("LOAD %s\n", (char *) instructions[i]);
                break;
            default:
                printf("***non-printable instruction***\n");
        }
    }
}

void print_stack(struct Stack *stack)
{
    printf("[ ");
    for (int i = 0; i < stack->offset; i++) {
        printf("%li ", (long) stack->values[i]);
    }
    printf("]\n");
}

void print_heap(struct Heap *heap)
{
    printf("[");
    while (heap->name != NULL) {
        printf(" { %s: %li }, ", heap->name, heap->value);
        heap = heap->next;
    }
    printf("]\n");
}

/* AST printers. Currently used for debugging purposes but could be used for 
 * building a formatter in the future */
void print_expr(struct Expr *expr)
{
    printf("(");
    switch (expr->op) {
    case OP_OR:
        print_value(expr->val1);
        printf(" or ");
        print_value(expr->val2);
        break;
    case OP_AND:
        print_value(expr->val1);
        printf(" and ");
        print_value(expr->val2);
        break;
    case OP_EQEQ:
        print_value(expr->val1);
        printf(" == ");
        print_value(expr->val2);
        break;
    case OP_NOT_EQ:
        print_value(expr->val1);
        printf(" <> ");
        print_value(expr->val2);
        break;
    case OP_GREATER_EQ:
        print_value(expr->val1);
        printf(" >= ");
        print_value(expr->val2);
        break;
    case OP_GREATER:
        print_value(expr->val1);
        printf(" > ");
        print_value(expr->val2);
        break;
    case OP_LESS_EQ:
        print_value(expr->val1);
        printf(" <= ");
        print_value(expr->val2);
        break;
    case OP_LESS:
        print_value(expr->val1);
        printf(" < ");
        print_value(expr->val2);
        break;
    case OP_PLUS:
        print_value(expr->val1);
        printf(" + ");
        print_value(expr->val2);
        break;
    case OP_MINUS:
        print_value(expr->val1);
        printf(" - ");
        print_value(expr->val2);
        break;
    case OP_STAR:
        print_value(expr->val1);
        printf(" * ");
        print_value(expr->val2);
        break;
    case OP_SLASH:
        print_value(expr->val1);
        printf(" / ");
        print_value(expr->val2);
        break;
    case OP_UNARY_PLUS:
        printf("+");
        print_value(expr->val1);
        break;
    case OP_UNARY_MINUS:
        printf("-");
        print_value(expr->val1);
        break;
    }
    printf(")");
}

void print_value(struct Value *val)
{
    switch (val->type) {
    case VTYPE_SYMBOL:
        printf("%s", val->symbol);
        break;
    case VTYPE_STRING:
        printf("\"%s\"", val->string);
        break;
    case VTYPE_INT:
        printf("%ld", val->integer);
        break;
    case VTYPE_FLOAT:
        printf("%f", val->floating);
        break;
    case VTYPE_BOOL:
        printf("%ld", val->boolean);
        break;
    case VTYPE_EXPR:
        print_expr(val->expr);
        break;
    case VTYPE_FUNCALL:
        printf("%s", val->funcall->funname);
        printf("(");
        for (Values *vals = val->funcall->values; vals != NULL;
                vals = vals->next) {
            print_value(vals->value);
            if (vals->next != NULL) printf(", ");
        }
        printf(")");
        break;
    }
}

static void print_statements_indent(Statements *stmts, int indent);

/* :) */
static void left_pad(int indent) {
    for (int i = 0; i < indent; i++) putchar(' ');
}

static const int TAB_WIDTH = 4;

static void print_statement_indent(struct Statement *stmt, int indent)
{
    Dim *dim = NULL;
    struct Definition *def = NULL;
    left_pad(indent);
    switch (stmt->type) {
        case STMT_DIM:
            dim = stmt->dim;
            printf("dim ");
            while (dim != NULL) {
                def = (struct Definition *) dim->value;
                printf("%s as ", def->name);
                switch (def->type) {
                    case TYPE_INT:
                        printf("int");
                        break;
                    case TYPE_FLOAT:
                        printf("float");
                        break;
                    case TYPE_BOOL:
                        printf("bool");
                        break;
                    case TYPE_STRING:
                        printf("string");
                        break;
                }
                if (dim->next) printf(", ");
                dim = dim->next;
            }
            break;
        case STMT_SET:
            printf("%s = ", stmt->set->symbol);
            print_value(stmt->set->val);
            break;
        case STMT_IF:
            printf("if ");
            print_value(stmt->if_stmt->condition);
            printf(" then\n");
            print_statements_indent(stmt->if_stmt->if_stmts, indent + TAB_WIDTH);
            if (stmt->if_stmt->else_stmts) {
                left_pad(indent);
                printf("else\n");
                print_statements_indent(stmt->if_stmt->else_stmts, indent + TAB_WIDTH);
            }
            printf("end if");
            break;
        case STMT_WHILE:
            printf("while ");
            print_value(stmt->while_stmt->condition);
            printf("\n");
            print_statements_indent(stmt->while_stmt->stmts, indent + TAB_WIDTH);
            printf("end while");
            break;
        case STMT_FOR:
        case STMT_FOREACH:
        case STMT_FUNCTION_DEF:
        case STMT_EXIT_WHILE:
        case STMT_EXIT_FOR:
        case STMT_EXIT_FUNCTION:
            printf("print_statement not yet implemented for this type");
            break;
        case STMT_FUNCALL:
            printf("%s", stmt->funcall->funname);
            printf("(");
            for (Values *vals = stmt->funcall->values; vals != NULL;
                    vals = vals->next) {
                print_value(vals->value);
                if (vals->next != NULL) printf(", ");
            }
            printf(")");
            break;
    }
    printf("\n");
}

static void print_statements_indent(Statements *stmts, int indent)
{
    while (stmts != NULL)
    {
        print_statement_indent(stmts->value, indent);
        stmts = stmts->next;
    }
}

void print_statement(struct Statement *stmt)
{
    print_statement_indent(stmt, 0);
}

void print_statements(Statements *stmts)
{
    print_statements_indent(stmts, 0);
}

