#include "common.h"
#include "linkedlist.h"
#include "printers.h"
#include "vector.h"

static void print_statements_indent(struct Globals *globals, Statements *stmts, int indent);
static void print_definitions(struct Globals *globals, struct LinkedList *lst, char sep, int indent);
static void print_statement_indent(struct Globals *globals, struct Statement *stmt, int indent);

static const int TAB_WIDTH = 4;

void print_instructions(struct Vector *instructions)
{
    size_t length = instructions->length;
    for (size_t i = 0; i < length; i++) {
        printf("%-5lu", i);
        switch (instructions->values[i].opcode) {
            case PUSH:
                i++;
                printf("PUSH %" PRIu64 "\n", instructions->values[i].integer);
                break;
            case PUSH_0:
                printf("PUSH_0\n");
                break;
            case PUSH_1:
                printf("PUSH_1\n");
                break;
            case PUSH_2:
                printf("PUSH_2\n");
                break;
            case PUSH_3:
                printf("PUSH_3\n");
                break;
            case PUSH_HEAP:
                printf("PUSH_HEAP\n");
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
            case MOD:
                printf("MOD\n");
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
            case CALL:
                printf("CALL\n");
                break;
            case GET_LOCAL:
                i++;
                printf("GET_LOCAL %" PRIu64 "\n", instructions->values[i].offset);
                break;
            case GET_LOCAL_0:
                printf("GET_LOCAL_0\n");
                break;
            case GET_LOCAL_1:
                printf("GET_LOCAL_1\n");
                break;
            case GET_LOCAL_2:
                printf("GET_LOCAL_2\n");
                break;
            case GET_LOCAL_3:
                printf("GET_LOCAL_3\n");
                break;
            case SET_LOCAL:
                printf("SET_LOCAL\n");
                break;
            case SET_LOCAL_0:
                printf("SET_LOCAL_0\n");
                break;
            case SET_LOCAL_1:
                printf("SET_LOCAL_1\n");
                break;
            case SET_LOCAL_2:
                printf("SET_LOCAL_2\n");
                break;
            case SET_LOCAL_3:
                printf("SET_LOCAL_3\n");
                break;
            case GET_HEAP:
                printf("GET_HEAP\n");
                break;
            case SET_HEAP:
                printf("SET_HEAP\n");
                break;
            case EXIT:
                printf("EXIT\n");
                break;
            case SWAP:
                printf("SWAP\n");
                break;
            case PUSH_SCOPE:
                printf("PUSH_SCOPE\n");
                break;
            case POP_SCOPE:
                printf("POP_SCOPE\n");
                break;
            case DEFINE:
                printf("DEFINE\n");
                break;
            case PRINT:
                printf("PRINT\n");
                break;
            case READ:
                printf("READ\n");
                break;
            default:
                printf("***non-printable instruction: %d***\n", instructions->values[i].opcode);
        }
    }
}

void print_stack(struct Stack *stack)
{
    printf("stack: %lu [ ", stack->offset);
    for (size_t i = 0; i < stack->offset; i++) {
        // If it would be interpreted as a huge number, it's probably a negative integer
        if (stack->values[i].integer > INT64_MAX) {
            printf("%" PRIi64 " ", (int64_t) stack->values[i].integer);
        } else {
            printf("%" PRIu64 " ", (uint64_t) stack->values[i].offset);
        }
    }
    printf("]\n");
}

void print_frames(struct StackFrames *sfs)
{
    printf("all variables:");
    for (size_t i = 0; i < sfs->index; i++) {
        // printf(" { %s: ", lookup_symbol(globals, sfs->names[i]));
        printf(" { %lu: ", i);
        printf("%" PRIi64 " }, ", (int64_t) sfs->values[i].integer);
    }
    printf("\n");
    printf("frame offsets: ");
    for (size_t i = 0; i < sfs->frame_offsets_index; i++) {
        printf("%lu, ", sfs->frame_offsets[i]);
    }
    printf("\n");
    size_t lower = 0;
    for (size_t i = 1; i <= sfs->frame_offsets_index; i++) {
        size_t upper = i == sfs->frame_offsets_index
                  ? sfs->index
                  : sfs->frame_offsets[i];
        printf("frame %ld: [ ", (unsigned long) i);
        for (size_t j = lower; j < upper; j++) {
            // printf(" { %s: ", lookup_symbol(globals, sfs->names[j]));
            printf(" { %lu: ", j);
            printf("%" PRIi64 " }, ", sfs->values[j].integer);
        }
        printf("] \n");
        lower = upper;
    }
}

void print_heap(struct Heap *heap)
{
    printf("heap:   [ memory usage: %lu, objcount: %ld, offset: %ld, values: { ", 
            IN_BYTES(heap->vector->length), heap->objcount, heap->vector->length);
    for (size_t i = 0; i < heap->vector->length; i++) {
        printf("%" PRIu64 "", heap->vector->values[i].integer);
        if (i != heap->vector->length - 1) printf(", ");
    }
    printf(" } ]\n");
}

// /* Typechecking related printers */
// void print_class_table(struct ClassType *table, uint64_t length)
// {
//     for (uint64_t i = 0; i < length; i++) {
//         struct ClassType clst = table[i];
//         printf("class %s {\n", lookup_symbol(globals, clst.name));
//         print_definitions(clst.definitions, ';', TAB_WIDTH);
//         // for (Methods *methods = cls->methods; methods != NULL; methods = methods->next) {
//         //     struct FunDef *method = (struct FunDef *) methods->value;
//         //     print_fundef(method, indent + TAB_WIDTH, 1);
//         //     printf("\n");
//         // }
//         printf("}");
//         // struct ClassType clst = table[i];
//         // printf("%" PRIu64 ": class %s {\n", i, lookup_symbol(globals, clst.name));
//         // print_definitions(lst, sep, )
//         // for (uint64_t j = 0; j < clst.count; j++) {
//         //     printf("    %s;\n", lookup_symbol(globals, clst.types[j]));
//         // }
//         // printf("}\n");
//     }
// }
// 
// void print_function_table(struct FunctionType *table, uint64_t length)
// {
//     for (uint64_t i = 0; i < length; i++) {
//         struct FunctionType ft = table[i];
//         printf("function %s(", lookup_symbol(globals, ft.name));
//         print_definitions(ft.definitions, ',', 0);
//         printf(") {\n");
//         // print_statements_indent(ft.stmts, TAB_WIDTH);
//         printf("}");
//     }
//     //for (uint64_t i = 0; i < length; i++) {
//     //    struct FunctionType ft = table[i];
//     //    printf("%" PRIu64 ": function %s(", i, lookup_symbol(globals, ft.name));
//     //    for (uint64_t j = 0; j < ft.count; j++) {
//     //        printf("%s", lookup_symbol(globals, ft.types[j]));
//     //        if (j != ft.count - 1) printf(", ");
//     //    }
//     //    printf(")\n");
//     //}
// }


/* AST printers. Currently used for debugging purposes but could be used for 
 * building a formatter in the future */
void print_expr(struct Globals *globals, struct Expr *expr)
{
    printf("(");
    switch (expr->op) {
    case OP_OR:
        print_value(globals, expr->val1);
        printf(" || ");
        print_value(globals, expr->val2);
        break;
    case OP_AND:
        print_value(globals, expr->val1);
        printf(" && ");
        print_value(globals, expr->val2);
        break;
    case OP_EQEQ:
        print_value(globals, expr->val1);
        printf(" == ");
        print_value(globals, expr->val2);
        break;
    case OP_NOT_EQ:
        print_value(globals, expr->val1);
        printf(" != ");
        print_value(globals, expr->val2);
        break;
    case OP_GREATER_EQ:
        print_value(globals, expr->val1);
        printf(" >= ");
        print_value(globals, expr->val2);
        break;
    case OP_GREATER:
        print_value(globals, expr->val1);
        printf(" > ");
        print_value(globals, expr->val2);
        break;
    case OP_LESS_EQ:
        print_value(globals, expr->val1);
        printf(" <= ");
        print_value(globals, expr->val2);
        break;
    case OP_LESS:
        print_value(globals, expr->val1);
        printf(" < ");
        print_value(globals, expr->val2);
        break;
    case OP_PLUS:
        print_value(globals, expr->val1);
        printf(" + ");
        print_value(globals, expr->val2);
        break;
    case OP_MINUS:
        print_value(globals, expr->val1);
        printf(" - ");
        print_value(globals, expr->val2);
        break;
    case OP_STAR:
        print_value(globals, expr->val1);
        printf(" * ");
        print_value(globals, expr->val2);
        break;
    case OP_SLASH:
        print_value(globals, expr->val1);
        printf(" / ");
        print_value(globals, expr->val2);
        break;
    case OP_UNARY_PLUS:
        printf("+");
        print_value(globals, expr->val1);
        break;
    case OP_UNARY_MINUS:
        printf("-");
        print_value(globals, expr->val1);
        break;
    default:
        assert("Error, not implemented" && false);
        break;
    }
    printf(")");
}

static void print_funcall(struct Globals *globals, Symbol funname, Values *vals)
{
    printf("%s", lookup_symbol(globals, funname));
    printf("(");
    if (vals != NULL) {
        linkedlist_foreach(lnode, vals->head) {
            print_value(globals, lnode->value);
            if (lnode->next != NULL) printf(", ");
        }
    }
    printf(")");
}

static void print_get(struct Globals *globals, struct Accessor *get)
{
    printf("%s", lookup_symbol(globals, get->definition->name));
    linkedlist_foreach(lnode, get->lvalues->head) {
    // for (LValues *lvalues = get->lvalues; lvalues != NULL; lvalues = lvalues->next) {
        struct LValue *lvalue = lnode->value;
        if (lvalue->type == LV_PROPERTY) {
            printf(".%s", lookup_symbol(globals, lvalue->property));
        } else {
            printf("[");
            print_value(globals, lvalue->index);
            printf("]");
        }
    }
}

void print_value(struct Globals *globals, struct Value *val)
{
    switch (val->vtype) {
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
    case VTYPE_NULL:
        printf("null");
        break;
    case VTYPE_EXPR:
        print_expr(globals, val->expr);
        break;
    case VTYPE_BUILTIN_FUNCALL:
    case VTYPE_FUNCALL:
        print_funcall(globals, val->funcall->funname, val->funcall->args);
        break;
    case VTYPE_BUILTIN_CONSTRUCTOR:
    case VTYPE_CONSTRUCTOR:
        printf("new ");
        print_funcall(globals, val->constructor->funname, val->constructor->args);
        break;
    case VTYPE_GET:
        print_get(globals, val->get);
        break;
    default:
        assert("Error, not implemented" && false);
        break;
    }
}

/* :) */
static void left_pad(int indent) {
    for (int i = 0; i < indent; i++) putchar(' ');
}

static void print_definitions(struct Globals *globals, struct LinkedList *lst, char sep, int indent)
{
    struct Definition *def = NULL;
    linkedlist_foreach(lnode, lst->head) {
        def = lnode->value;
        if (sep == ';') left_pad(indent);
        printf("%s: ", lookup_symbol(globals, def->name));
        printf("%s", lookup_symbol(globals, def->type));
        if (sep == ';') {
            printf(";\n");
        } else if (lnode->next && sep == ',') {
            printf(", ");
        }
    }
    // while (lst != NULL) {
    //     def = (struct Definition *) lst->value;
    //     if (sep == ';') left_pad(indent);
    //     printf("%s: ", lookup_symbol(globals, def->name));
    //     printf("%s", lookup_symbol(globals, def->type));
    //     if (sep == ';') {
    //         printf(";\n");
    //     } else if (lst->next && sep == ',') {
    //         printf(", ");
    //     }
    //     lst = lst->next;
    // }
}

static void print_set(struct Globals *globals, struct Set *set)
{
    if (set->is_define) printf("let ");
    Symbol symbol = set->to_set->definition->name;
    char *strsym = lookup_symbol(globals, symbol);
    printf("%s", strsym);
    if (set->to_set->lvalues != NULL) {
        linkedlist_foreach(lnode, set->to_set->lvalues->head) {
        // for (LValues *lvalues = set->to_set->lvalues; lvalues != NULL; lvalues = lvalues->next) {
            struct LValue *lvalue = lnode->value;
            if (lvalue->type == LV_PROPERTY) {
                printf(".%s", lookup_symbol(globals, lvalue->property));
            } else {
                printf("[");
                print_value(globals, lvalue->index);
                printf("]");
            }
        }
    }
    printf(" = ");
    print_value(globals, set->val);
    printf(";");
}

static void print_if(struct Globals *globals, struct IfStatement *if_stmt, int indent)
{
    printf("if ");
    print_value(globals, if_stmt->condition);
    printf(" {\n");
    print_statements_indent(globals, if_stmt->if_stmts, indent + TAB_WIDTH);
    if (if_stmt->else_stmts) {
        left_pad(indent);
        printf("} else {\n");
        print_statements_indent(globals, if_stmt->else_stmts, indent + TAB_WIDTH);
    }
    left_pad(indent);
    printf("}");

}

// struct Statement *init;
// struct Value *condition;
// struct Statement *increment;
// Statements *stmts;
static void print_for(struct Globals *globals, struct For *for_stmt, int indent)
{
    printf("for (\n");
    print_statement_indent(globals, for_stmt->init, indent + TAB_WIDTH);
    left_pad(indent + TAB_WIDTH);
    print_value(globals, for_stmt->condition);
    printf(";\n");
    print_statement_indent(globals, for_stmt->increment, indent + TAB_WIDTH);
    left_pad(indent);
    printf(") {\n");
    print_statements_indent(globals, for_stmt->stmts, indent + TAB_WIDTH);
    left_pad(indent);
    printf("}");
}

static void print_while(struct Globals *globals, struct While *while_stmt, int indent)
{
    printf("while ");
    print_value(globals, while_stmt->condition);
    printf(" {\n");
    print_statements_indent(globals, while_stmt->stmts, indent + TAB_WIDTH);
    left_pad(indent);
    printf("}");
}

static void print_statement_indent(struct Globals *globals, struct Statement *stmt, int indent)
{
    left_pad(indent);
    switch (stmt->type) {
        case STMT_LET:
            printf("let ");
            print_definitions(globals, stmt->let, ',', indent);
            break;
        case STMT_SET:
            print_set(globals, stmt->set);
            break;
        case STMT_IF:
            print_if(globals, stmt->if_stmt, indent);
            break;
        case STMT_WHILE:
            print_while(globals, stmt->while_stmt, indent);
            break;
        case STMT_RETURN:
            printf("return");
            if (stmt->ret != NULL) {
                printf(" ");
                print_value(globals, stmt->ret);
            }
            printf(";");
            break;
        case STMT_FOR:
            print_for(globals, stmt->for_stmt, indent);
            break;
        case STMT_FOREACH:
            printf("print_statement not yet implemented for this type");
            break;
        case STMT_BUILTIN_FUNCALL:
        case STMT_FUNCALL:
            printf("%s", lookup_symbol(globals, stmt->funcall->funname));
            printf("(");
            if (stmt->funcall->args != NULL) {
                linkedlist_foreach(lnode, stmt->funcall->args->head) {
                    print_value(globals, lnode->value);
                    if (lnode->next != NULL) printf(", ");
                }
            }
            printf(");");
            break;
        default:
            assert("Error, not implemented" && false);
            break;
    }
    printf("\n");
}

static void print_statements_indent(struct Globals *globals, Statements *stmts, int indent)
{
    linkedlist_foreach(stmt, stmts->head) {
        print_statement_indent(globals, stmt->value, indent);
    }
}

void print_statement(struct Globals *globals, struct Statement *stmt)
{
    print_statement_indent(globals, stmt, 0);
}

void print_statements(struct Globals *globals, Statements *stmts)
{
    print_statements_indent(globals, stmts, 0);
}

void print_fundef(struct Globals *globals, struct FunDef *fundef, int indent, int ismethod)
{
    left_pad(indent);
    if (!ismethod) {
        printf("function ");
    }
    printf("%s(", lookup_symbol(globals, fundef->name));
    print_definitions(globals, fundef->args, ',', indent);
    printf("): %s {\n", lookup_symbol(globals, fundef->rettype));
    print_statements_indent(globals, fundef->stmts, TAB_WIDTH + indent);
    left_pad(indent);
    printf("}");
}

void print_class(struct Globals *globals, struct Class *cls, int indent)
{
    printf("class %s {\n", lookup_symbol(globals, cls->name));
    print_definitions(globals, cls->definitions, ';', indent + TAB_WIDTH);
    // for (Methods *methods = cls->methods; methods != NULL; methods = methods->next) {
    //     struct FunDef *method = (struct FunDef *) methods->value;
    //     print_fundef(method, indent + TAB_WIDTH, 1);
    //     printf("\n");
    // }
    printf("}");
}

static void print_tld_indent(struct Globals *globals, struct TopLevelDecl *tld, int indent)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            print_class(globals, tld->cls, indent);
            break;
        case TLD_TYPE_FUNDEF:
            print_fundef(globals, tld->fundef, indent, 0);
            break;
    }
    printf("\n\n");
}

static void print_tlds_indent(struct Globals *globals, TopLevelDecls *tlds, int indent)
{
    printf("%" PRIu64 " functions defined\n", globals->function_count);
    printf("%" PRIu64 " classes defined\n", globals->class_count);
    // while (tlds != NULL)
    linkedlist_foreach(tld, tlds->head) {
        print_tld_indent(globals, tld->value, indent);
    }
}

void print_tlds(struct Globals *globals, TopLevelDecls *tlds)
{
    print_tlds_indent(globals, tlds, 0);
}

void print_ft_node(struct Globals *globals, struct FunctionCallTableNode *fn)
{
    printf("%s: ", lookup_symbol(globals, fn->function));
    linkedlist_foreach(lnode, fn->callsites->head) {
    // for (struct List *calls = fn->callsites; calls != NULL; calls = calls->next) {
        printf("%" PRIu64 "", *((uint64_t *) lnode->value));
        if (lnode->next != NULL) printf(", ");
    }
    printf("\n");
}

void print_ft(struct Globals *globals, struct FunctionCallTable *ft)
{
    if (ft == NULL) return;
    print_ft_node(globals, ft->node);
    print_ft(globals, ft->left);
    print_ft(globals, ft->right);
}

void print_binary_helper(uint64_t num, size_t length)
{
    size_t bit_width = length * 8;
    for (int i = bit_width - 1; i >= 0; i--) {
        uint64_t part = (num & (UINT64_C(1) << i)) >> i;
        if (part == 1) {
            printf("1");
        } else {
            printf("0");
        }
        if (i != 0 && i % 8 == 0) {
            putchar('_');
        }
    }
    printf("\n");
}

