#include "common.h"
#include "linkedlist.h"
#include "printers.h"

static void print_statements_indent(Statements *stmts, int indent);
static void print_definitions(struct LinkedList *lst, char sep, int indent);
static void print_statement_indent(struct Statement *stmt, int indent);

static const int TAB_WIDTH = 4;

void print_instructions(struct CompilerContext *cc)
{
    int length = cc->offset;
    DelValue *instructions = cc->instructions;
    for (int i = 0; i < length; i++) {
        printf("%-5d", i);
        switch (instructions[i].opcode) {
            case PUSH:
                i++;
                printf("PUSH %" PRIu64 "\n", instructions[i].integer);
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
                printf("GET_LOCAL %" PRIu64 "\n", instructions[i].offset);
                break;
            case SET_LOCAL:
                printf("SET_LOCAL\n");
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
            default:
                printf("***non-printable instruction***\n");
        }
    }
}

void print_stack(struct Stack *stack)
{
    printf("stack:  [ ");
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
        // printf(" { %s: ", lookup_symbol(sfs->names[i]));
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
            // printf(" { %s: ", lookup_symbol(sfs->names[j]));
            printf(" { %lu: ", j);
            printf("%" PRIi64 " }, ", sfs->values[j].integer);
        }
        printf("] \n");
        lower = upper;
    }
}

void print_heap(struct Heap *heap)
{
    printf("heap:   [ objcount: %ld, offset: %ld, values: { ", heap->objcount, heap->offset);
    for (size_t i = 0; i < heap->offset; i++) {
        printf("%" PRIu64 "", heap->values[i].integer);
        if (i != heap->offset - 1) printf(", ");
    }
    printf(" } ]\n");
}

// /* Typechecking related printers */
// void print_class_table(struct ClassType *table, uint64_t length)
// {
//     for (uint64_t i = 0; i < length; i++) {
//         struct ClassType clst = table[i];
//         printf("class %s {\n", lookup_symbol(clst.name));
//         print_definitions(clst.definitions, ';', TAB_WIDTH);
//         // for (Methods *methods = cls->methods; methods != NULL; methods = methods->next) {
//         //     struct FunDef *method = (struct FunDef *) methods->value;
//         //     print_fundef(method, indent + TAB_WIDTH, 1);
//         //     printf("\n");
//         // }
//         printf("}");
//         // struct ClassType clst = table[i];
//         // printf("%" PRIu64 ": class %s {\n", i, lookup_symbol(clst.name));
//         // print_definitions(lst, sep, )
//         // for (uint64_t j = 0; j < clst.count; j++) {
//         //     printf("    %s;\n", lookup_symbol(clst.types[j]));
//         // }
//         // printf("}\n");
//     }
// }
// 
// void print_function_table(struct FunctionType *table, uint64_t length)
// {
//     for (uint64_t i = 0; i < length; i++) {
//         struct FunctionType ft = table[i];
//         printf("function %s(", lookup_symbol(ft.name));
//         print_definitions(ft.definitions, ',', 0);
//         printf(") {\n");
//         // print_statements_indent(ft.stmts, TAB_WIDTH);
//         printf("}");
//     }
//     //for (uint64_t i = 0; i < length; i++) {
//     //    struct FunctionType ft = table[i];
//     //    printf("%" PRIu64 ": function %s(", i, lookup_symbol(ft.name));
//     //    for (uint64_t j = 0; j < ft.count; j++) {
//     //        printf("%s", lookup_symbol(ft.types[j]));
//     //        if (j != ft.count - 1) printf(", ");
//     //    }
//     //    printf(")\n");
//     //}
// }


/* AST printers. Currently used for debugging purposes but could be used for 
 * building a formatter in the future */
void print_expr(struct Expr *expr)
{
    printf("(");
    switch (expr->op) {
    case OP_OR:
        print_value(expr->val1);
        printf(" || ");
        print_value(expr->val2);
        break;
    case OP_AND:
        print_value(expr->val1);
        printf(" && ");
        print_value(expr->val2);
        break;
    case OP_EQEQ:
        print_value(expr->val1);
        printf(" == ");
        print_value(expr->val2);
        break;
    case OP_NOT_EQ:
        print_value(expr->val1);
        printf(" != ");
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
    default:
        assert("Error, not implemented" && false);
        break;
    }
    printf(")");
}

static void print_funcall(Symbol funname, Values *vals)
{
    printf("%s", lookup_symbol(funname));
    printf("(");
    if (vals != NULL) {
        linkedlist_foreach(lnode, vals->head) {
            print_value(lnode->value);
            if (lnode->next != NULL) printf(", ");
        }
    }
    printf(")");
}

static void print_get(struct Accessor *get)
{
    printf("%s", lookup_symbol(get->definition->name));
    linkedlist_foreach(lnode, get->lvalues->head) {
    // for (LValues *lvalues = get->lvalues; lvalues != NULL; lvalues = lvalues->next) {
        struct LValue *lvalue = lnode->value;
        if (lvalue->type == LV_PROPERTY) {
            printf(".%s", lookup_symbol(lvalue->property));
        } else {
            printf("[");
            print_value(lvalue->index);
            printf("]");
        }
    }
}

void print_value(struct Value *val)
{
    switch (val->vtype) {
    // case VTYPE_SYMBOL:
    //     printf("%s", lookup_symbol(val->symbol));
    //     break;
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
        print_funcall(val->funcall->funname, val->funcall->args);
        break;
    case VTYPE_CONSTRUCTOR:
        printf("new ");
        print_funcall(val->constructor->funname, val->constructor->args);
        break;
    case  VTYPE_GET:
        print_get(val->get);
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

static void print_definitions(struct LinkedList *lst, char sep, int indent)
{
    struct Definition *def = NULL;
    linkedlist_foreach(lnode, lst->head) {
        def = lnode->value;
        if (sep == ';') left_pad(indent);
        printf("%s: ", lookup_symbol(def->name));
        printf("%s", lookup_symbol(def->type));
        if (sep == ';') {
            printf(";\n");
        } else if (lnode->next && sep == ',') {
            printf(", ");
        }
    }
    // while (lst != NULL) {
    //     def = (struct Definition *) lst->value;
    //     if (sep == ';') left_pad(indent);
    //     printf("%s: ", lookup_symbol(def->name));
    //     printf("%s", lookup_symbol(def->type));
    //     if (sep == ';') {
    //         printf(";\n");
    //     } else if (lst->next && sep == ',') {
    //         printf(", ");
    //     }
    //     lst = lst->next;
    // }
}

static void print_set(struct Set *set)
{
    if (set->is_define) printf("let ");
    Symbol symbol = set->to_set->definition->name;
    char *strsym = lookup_symbol(symbol);
    printf("%s", strsym);
    if (set->to_set->lvalues != NULL) {
        linkedlist_foreach(lnode, set->to_set->lvalues->head) {
        // for (LValues *lvalues = set->to_set->lvalues; lvalues != NULL; lvalues = lvalues->next) {
            struct LValue *lvalue = lnode->value;
            if (lvalue->type == LV_PROPERTY) {
                printf(".%s", lookup_symbol(lvalue->property));
            } else {
                printf("[");
                print_value(lvalue->index);
                printf("]");
            }
        }
    }
    printf(" = ");
    print_value(set->val);
    printf(";");
}

static void print_if(struct IfStatement *if_stmt, int indent)
{
    printf("if ");
    print_value(if_stmt->condition);
    printf(" {\n");
    print_statements_indent(if_stmt->if_stmts, indent + TAB_WIDTH);
    if (if_stmt->else_stmts) {
        left_pad(indent);
        printf("} else {\n");
        print_statements_indent(if_stmt->else_stmts, indent + TAB_WIDTH);
    }
    left_pad(indent);
    printf("}");

}

// struct Statement *init;
// struct Value *condition;
// struct Statement *increment;
// Statements *stmts;
static void print_for(struct For *for_stmt, int indent)
{
    printf("for (\n");
    print_statement_indent(for_stmt->init, indent + TAB_WIDTH);
    left_pad(indent + TAB_WIDTH);
    print_value(for_stmt->condition);
    printf(";\n");
    print_statement_indent(for_stmt->increment, indent + TAB_WIDTH);
    left_pad(indent);
    printf(") {\n");
    print_statements_indent(for_stmt->stmts, indent + TAB_WIDTH);
    left_pad(indent);
    printf("}");
}

static void print_while(struct While *while_stmt, int indent)
{
    printf("while ");
    print_value(while_stmt->condition);
    printf(" {\n");
    print_statements_indent(while_stmt->stmts, indent + TAB_WIDTH);
    left_pad(indent);
    printf("}");
}

static void print_statement_indent(struct Statement *stmt, int indent)
{
    left_pad(indent);
    switch (stmt->type) {
        case STMT_LET:
            printf("let ");
            print_definitions(stmt->let, ',', indent);
            break;
        case STMT_SET:
            print_set(stmt->set);
            break;
        case STMT_IF:
            print_if(stmt->if_stmt, indent);
            break;
        case STMT_WHILE:
            print_while(stmt->while_stmt, indent);
            break;
        case STMT_RETURN:
            printf("return");
            if (stmt->ret != NULL) {
                printf(" ");
                print_value(stmt->ret);
            }
            printf(";");
            break;
        case STMT_FOR:
            print_for(stmt->for_stmt, indent);
            break;
        case STMT_FOREACH:
            printf("print_statement not yet implemented for this type");
            break;
        case STMT_FUNCALL:
            printf("%s", lookup_symbol(stmt->funcall->funname));
            printf("(");
            if (stmt->funcall->args != NULL) {
                linkedlist_foreach(lnode, stmt->funcall->args->head) {
                    print_value(lnode->value);
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

static void print_statements_indent(Statements *stmts, int indent)
{
    linkedlist_foreach(stmt, stmts->head) {
        print_statement_indent(stmt->value, indent);
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

void print_fundef(struct FunDef *fundef, int indent, int ismethod)
{
    left_pad(indent);
    if (!ismethod) {
        printf("function ");
    }
    printf("%s(", lookup_symbol(fundef->name));
    print_definitions(fundef->args, ',', indent);
    printf("): %s {\n", lookup_symbol(fundef->rettype));
    print_statements_indent(fundef->stmts, TAB_WIDTH + indent);
    left_pad(indent);
    printf("}");
}

void print_class(struct Class *cls, int indent)
{
    printf("class %s {\n", lookup_symbol(cls->name));
    print_definitions(cls->definitions, ';', indent + TAB_WIDTH);
    // for (Methods *methods = cls->methods; methods != NULL; methods = methods->next) {
    //     struct FunDef *method = (struct FunDef *) methods->value;
    //     print_fundef(method, indent + TAB_WIDTH, 1);
    //     printf("\n");
    // }
    printf("}");
}

static void print_tld_indent(struct TopLevelDecl *tld, int indent)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            print_class(tld->cls, indent);
            break;
        case TLD_TYPE_FUNDEF:
            print_fundef(tld->fundef, indent, 0);
            break;
    }
    printf("\n\n");
}

static void print_tlds_indent(TopLevelDecls *tlds, int indent)
{
    printf("%" PRIu64 " functions defined\n", globals.function_count);
    printf("%" PRIu64 " classes defined\n", globals.class_count);
    // while (tlds != NULL)
    linkedlist_foreach(tld, tlds->head) {
        print_tld_indent(tld->value, indent);
    }
}

void print_tlds(TopLevelDecls *tlds)
{
    print_tlds_indent(tlds, 0);
}

void print_ft_node(struct FunctionCallTableNode *fn)
{
    printf("%s: ", lookup_symbol(fn->function));
    linkedlist_foreach(lnode, fn->callsites->head) {
    // for (struct List *calls = fn->callsites; calls != NULL; calls = calls->next) {
        printf("%" PRIu64 "", *((uint64_t *) lnode->value));
        if (lnode->next != NULL) printf(", ");
    }
    printf("\n");
}

void print_ft(struct FunctionCallTable *ft)
{
    if (ft == NULL) return;
    print_ft_node(ft->node);
    print_ft(ft->left);
    print_ft(ft->right);
}

