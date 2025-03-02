#include "common.h"
#include "linkedlist.h"
#include "printers.h"
#include "vector.h"

static void print_statements_indent(struct Globals *globals, Statements *stmts, int indent);
static void print_definitions(struct Globals *globals, struct LinkedList *lst, char sep, int indent);
static void print_statement_indent(struct Globals *globals, struct Statement *stmt, int indent);

static const int TAB_WIDTH = 4;

void print_instructions(struct CompilerContext *cc)
{
    struct Vector *instructions = cc->instructions;
    struct LinkedList *comments = cc->comments;
    linkedlist_reverse(&comments);
    size_t length = instructions->length;
    struct Comment *comment = NULL;
    if (!linkedlist_is_empty(comments)) {
        comment = linkedlist_pop(comments);
    }
    size_t index;
    DelValue val1, val2;
    for (size_t i = 0; i < length; i++) {
        while (comment != NULL && comment->location == i) {
            printf("// %s\n", comment->comment);
            if (linkedlist_is_empty(comments)) {
                comment = NULL;
            } else {
                comment = linkedlist_pop(comments);
            }
        }
        printf("%-5lu", i);
        switch (instructions->values[i].opcode) {
            case PUSH:
                i++;
                printf("PUSH %" PRIu64 "\n", instructions->values[i].integer);
                break;
            case PUSH_OBJ:
                i++;
                printf("PUSH_OBJ %" PRIu64 "\n", instructions->values[i].integer);
                break;
            case DUP:
                printf("DUP\n");
                break;
            case PUSH_HEAP:
                i++;
                val1 = instructions->values[i];
                i++;
                val2 = instructions->values[i];
                printf("PUSH_HEAP %lu (count), %lu (metadata)\n", val1.offset, val2.offset);
                break;
            case PUSH_ARRAY:
                printf("PUSH_ARRAY\n");
                break;
            case LEN_ARRAY:
                printf("LEN_ARRAY\n");
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
            case FLOAT_ADD:
                printf("FLOAT_ADD\n");
                break;
            case FLOAT_SUB:
                printf("FLOAT_SUB\n");
                break;
            case FLOAT_MUL:
                printf("FLOAT_MUL\n");
                break;
            case FLOAT_DIV:
                printf("FLOAT_DIV\n");
                break;
            case JE:
                printf("JE\n");
                break;
            case JMP:
                i++;
                index = instructions->values[i].offset;
                printf("JMP %" PRIu64 "\n", index);
                break;
            case JNE:
                i++;
                index = instructions->values[i].offset;
                printf("JNE %" PRIu64 "\n", index);
                break;
            case RET:
                printf("RET\n");
                break;
            case POP:
                printf("POP\n");
                break;
            case POP_OBJ:
                printf("POP_OBJ\n");
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
            case LTE:
                printf("LTE\n");
                break;
            case GT:
                printf("GT\n");
                break;
            case GTE: 
                printf("GTE\n");
                break;
            case FLOAT_EQ:
                printf("FLOAT_EQ\n");
                break;
            case FLOAT_NEQ:
                printf("FLOAT_NEQ\n");
                break;
            case FLOAT_LT:
                printf("FLOAT_LT\n");
                break;
            case FLOAT_LTE:
                printf("FLOAT_LTE\n");
                break;
            case FLOAT_GT:
                printf("FLOAT_GT\n");
                break;
            case FLOAT_GTE:
                printf("FLOAT_GTE\n");
                break;
            case AND:
                printf("AND\n");
                break;
            case OR:
                printf("OR\n");
                break;
            case UNARY_MINUS:
                printf("UNARY_MINUS\n");
                break;
            case FLOAT_UNARY_MINUS:
                printf("FLOAT_UNARY_MINUS\n");
                break;
            case CAST_INT:
                printf("CAST_INT");
                break;
            case CAST_FLOAT:
                printf("CAST_FLOAT");
                break;
            case CALL:
                i++;
                uint64_t num_args = instructions->values[i].offset;
                i++;
                void *context = (void *)instructions->values[i].pointer;
                i++;
                void *function = (void *)instructions->values[i].pointer;
                printf("CALL args %lu, context %p, function %p\n", num_args, context, function);
                break;
            case EQ_OBJ:
                printf("EQ_OBJ\n");
                break;
            case NEQ_OBJ:
                printf("NEQ_OBJ\n");
                break;
            case GET_LOCAL_OBJ:
                i++;
                printf("GET_LOCAL_OBJ %" PRIu64 "\n", instructions->values[i].offset);
                break;
            case GET_LOCAL:
                i++;
                printf("GET_LOCAL %" PRIu64 "\n", instructions->values[i].offset);
                break;
            case SET_LOCAL_OBJ:
                i++;
                printf("SET_LOCAL_OBJ %" PRIu64 "\n", instructions->values[i].offset);
                break;
            case SET_LOCAL:
                i++;
                printf("SET_LOCAL %" PRIu64 "\n", instructions->values[i].offset);
                break;
            case GET_HEAP:
                i++;
                index = instructions->values[i].offset;
                printf("GET_HEAP %" PRIu64 "\n", index - index / 4 - 1);
                break;
            case GET_HEAP_OBJ:
                i++;
                index = instructions->values[i].offset;
                printf("GET_HEAP_OBJ %" PRIu64 "\n", index - index / 4 - 1);
                break;
            case SET_HEAP:
                i++;
                index = instructions->values[i].offset;
                printf("SET_HEAP %" PRIu64 "\n", index - index / 4 - 1);
                break;
            case SET_HEAP_OBJ:
                i++;
                index = instructions->values[i].offset;
                printf("SET_HEAP_OBJ %" PRIu64 "\n", index - index / 4 - 1);
                break;
            case GET_ARRAY:
                printf("GET_ARRAY\n");
                break;
            case GET_ARRAY_OBJ:
                printf("GET_ARRAY_OBJ\n");
                break;
            case SET_ARRAY:
                printf("SET_ARRAY\n");
                break;
            case SET_ARRAY_OBJ:
                printf("SET_ARRAY_OBJ\n");
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
            case DEFINE_OBJ:
                printf("DEFINE_OBJ\n");
                break;
            case PRINT:
                printf("PRINT\n");
                break;
            case READ:
                printf("READ\n");
                break;
            default:
                printf("***non-printable instruction: %d***\n", instructions->values[i].opcode);
                assert(false);
        }
    }
}

void print_stack(struct Stack *stack, bool is_obj)
{
    printf("stack%s: %lu [ ", is_obj ? "_obj" : "", stack->offset);
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

void print_frames(struct StackFrames *sfs, bool is_obj)
{
    char *objstr = is_obj ? "object " : "";
    printf("all %svariables:", objstr);
    for (size_t i = 0; i < sfs->index; i++) {
        // printf(" { %s: ", lookup_symbol(globals, sfs->names[i]));
        printf(" { %lu: ", i);
        printf("%" PRIi64 " }, ", (int64_t) sfs->values[i].integer);
    }
    printf("\n");
    printf("%sframe offsets: ", objstr);
    for (size_t i = 0; i < sfs->frame_offsets_index; i++) {
        printf("%lu, ", sfs->frame_offsets[i]);
    }
    printf("\n");
    size_t lower = 0;
    for (size_t i = 1; i <= sfs->frame_offsets_index; i++) {
        size_t upper = i == sfs->frame_offsets_index
                  ? sfs->index
                  : sfs->frame_offsets[i];
        printf("%sframe %ld: [ ", objstr, (unsigned long) i);
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
    struct Vector *vector = heap->vector;
    printf("heap: [ gc threshold: %lu bytes, memory usage: %lu bytes, ",
            IN_BYTES(heap->gc_threshold),
            IN_BYTES(vector->length));
    printf("values: { "); 
    for (size_t i = 0; i < vector->length; i++) {
        printf("%" PRIu64 "", vector->values[i].integer);
        if (i != vector->length - 1) printf(", ");
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

static void print_get_property(struct Globals *globals, bool is_prop, struct GetProperty *get)
{
    print_value(globals, get->accessor);
    if (is_prop) {
        printf(".%s", lookup_symbol(globals, get->property));
    } else {
        printf("[");
        print_value(globals, get->index);
        printf("]");
    }
    return;
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
        print_funcall(globals, val->funcall->access->definition->name, val->funcall->args);
        break;
    case VTYPE_BUILTIN_CONSTRUCTOR:
    case VTYPE_CONSTRUCTOR:
        printf("new ");
        struct FunCall *funcall = val->constructor->funcall;
        print_funcall(globals, funcall->access->definition->name, funcall->args);
        break;
    case VTYPE_GET_LOCAL:
        printf("%s", lookup_symbol(globals, val->get_local->name));
        break;
    case VTYPE_GET_PROPERTY:
        print_get_property(globals, true, val->get_property);
        break;
    case VTYPE_INDEX:
        print_get_property(globals, false, val->get_property);
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
}

static void print_set_property(struct Globals *globals, bool is_prop, struct SetProperty *set)
{
    print_get_property(globals, is_prop, set->access);
    printf(" = ");
    print_value(globals, set->expr);
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
        case STMT_IF:
            print_if(globals, stmt->if_stmt, indent);
            break;
        case STMT_WHILE:
            print_while(globals, stmt->while_stmt, indent);
            break;
        case STMT_RETURN:
            printf("return");
            if (stmt->val != NULL) {
                printf(" ");
                print_value(globals, stmt->val);
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
            printf("%s", lookup_symbol(globals, stmt->funcall->access->definition->name));
            printf("(");
            if (stmt->funcall->args != NULL) {
                linkedlist_foreach(lnode, stmt->funcall->args->head) {
                    print_value(globals, lnode->value);
                    if (lnode->next != NULL) printf(", ");
                }
            }
            printf(");");
            break;
        case STMT_SET_LOCAL:
            if (stmt->set_local->is_define) {
                printf("let ");
            }
            printf("%s = ", lookup_symbol(globals, stmt->set_local->def->name));
            print_value(globals, stmt->set_local->expr);
            printf(";");
            break;
        case STMT_SET_PROPERTY:
            print_set_property(globals, true, stmt->set_property);
            break;
        case STMT_SET_INDEX:
            print_set_property(globals, false, stmt->set_property);
            break;
        case STMT_INC:
            print_value(globals, stmt->val);
            printf("++;");
            break;
        case STMT_DEC:
            print_value(globals, stmt->val);
            printf("--;");
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

void print_scope(struct Globals *globals, struct Scope *scope)
{
    if (scope->parent != NULL) {
        print_scope(globals, scope->parent);
    } else {
        printf("(Top Level)");
    }
    printf("<- %ld { ", scope->varcount);
    linkedlist_foreach(lnode, scope->definitions->head) {
        struct Definition *def = lnode->value;
        printf("%s: %s", lookup_symbol(globals, def->name), lookup_symbol(globals, def->type));
        if (lnode->next != NULL) {
            printf(", ");
        }
    }
    printf(" }\n");
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

// void print_reachable_child_objects(struct GarbageCollector *gc, HeapPointer ptr, struct Heap *heap, char **string_pool)
// {
//     if (ptr == 0 || gc_is_marked(ptr)) {
//         return;
//     }
//     gc_remap(gc, ptr);
//     gc_mark(&ptr);
//     size_t location = get_location(ptr);
//     size_t count = get_count(ptr);
//     // Loop through elements in heap object, mark inner objects
//     if (is_array_ptr(ptr)) {
//         if (is_array_of_objects(ptr)) {
//             printf("marking child array: \n");
//             for (size_t i = location; i < count + location; i++) {
//                 DelValue value = vector_get(heap->vector, i);
//                 gc_mark_children(gc, value.offset, heap, string_pool);
//             }
//         }
//     } else {
//         printf("marking child object: \n");
//         print_object(heap, location, count, string_pool);
//         printf("\n");
//         uint16_t types[4] = {0};
//         uint16_t type_index = 0;
//         for (size_t i = 0; i < count; i++) {
//             DelValue value = vector_get(heap->vector, i + location);
//             if (i % 5 == 0) {
//                 memcpy(types, value.types, 8);
//                 type_index = 0;
//             } else {
//                 Type type = types[type_index];
//                 if (is_object(type)) {
//                     printf("recursing...\n");
//                     gc_mark_children(gc, value.offset, heap, string_pool);
//                 }
//                 type_index++;
//             }
//         }
//     }
// }

// void print_reachable_objects(struct Heap *heap, struct Stack *stack, struct StackFrames *sfs, char **string_pool)
// {
//     Allocator allocator = allocator_new();
//     struct LinkedList *ll = linkedlist_new(allocator);
//     printf("stack pointers:\n");
//     for (size_t i = 0; i < stack->offset; i++) {
//         HeapPointer ptr = stack->values[i].offset;
//         linkedlist_append(ll,
//     }
//     printf("local variable pointers:\n");
//     for (size_t i = 0; i < sfs->index; i++) {
//         HeapPointer ptr = sfs->values[i].offset;
//         gc_mark_children(&gc, ptr, heap, string_pool);
//     }
//     printf("========================\n");
//     allocator_freeall(allocator);
//     return;
// }

