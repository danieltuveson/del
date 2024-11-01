#include "common.h"
#include "allocator.h"
#include "linkedlist.h"
#include "ast.h"
#include "printers.h"
#include "typecheck.h"

#define ALL_GOOD 1
#define BAD_STUFF 0

#define find_open_loc(i, table, length, symbol)\
    for (i = symbol % length; table[i].name != 0; i = i == length - 1 ? 0 : i + 1)

struct Scope {
    bool isfunction;
    size_t varcount;
    Definitions *definitions;
    struct Scope *parent;
};

struct TypeCheckerContext {
    struct FunDef *enclosing_func;
    struct FunctionTable *fun_table;
    struct ClassTable *cls_table;
    struct Scope *scope;
};

static bool typecheck_statements(struct TypeCheckerContext *context, Statements *stmts);
static bool typecheck_statement(struct TypeCheckerContext *context, struct Statement *stmt);
static bool typecheck_funcall(struct TypeCheckerContext *context, struct FunCall *funcall,
                              struct FunDef *fundef);
static Type typecheck_constructor(struct TypeCheckerContext *context, struct FunCall *constructor);

#define table_lookup(table, length, symbol)\
    uint64_t i = symbol % length;\
    for (uint64_t count = 0; count < length; count++) {\
        if (table[i].name == symbol) {\
            return &(table[i]);\
        }\
        i = i == length - 1 ? 0 : i + 1;\
    }\
    return NULL

struct Class *lookup_class(struct ClassTable *ct, Symbol symbol)
{
    table_lookup(ct->table, ct->size, symbol);
}

struct FunDef *lookup_fun(struct FunctionTable *ft, Symbol symbol)
{
    table_lookup(ft->table, ft->size, symbol);
}

#undef table_lookup

void print_scope(struct Scope *scope)
{
    if (scope->parent != NULL) {
        print_scope(scope->parent);
    } else {
        printf("(Top Level)");
    }
    printf("<- %ld { ", scope->varcount);
    linkedlist_foreach(lnode, scope->definitions->head) {
        struct Definition *def = lnode->value;
        printf("%s: %s", lookup_symbol(def->name), lookup_symbol(def->type));
        if (lnode->next != NULL) {
            printf(", ");
        }
    }
    printf(" }\n");
}

static void enter_scope(struct Scope **current, bool isfunction)
{
    struct Scope *scope = allocator_malloc(sizeof(*scope));
    scope->isfunction = isfunction;
    if (*current != NULL && !isfunction) {
        scope->varcount = (*current)->varcount;
    } else {
        scope->varcount = 0;
    }
    scope->definitions = linkedlist_new();
    scope->parent = *current;
    *current = scope;
}

static void exit_scope(struct Scope **current)
{
    *current = (*current)->parent;
}

static struct Definition *lookup_var(struct Scope *scope, Symbol name)
{
    linkedlist_foreach(lnode, scope->definitions->head) {
        struct Definition *def = lnode->value;
        if (def->name == name) {
            return def;
        }
    }
    if (scope->parent != NULL) {
        return lookup_var(scope->parent, name);
    }
    return NULL;
}

static bool add_var(struct Scope *current, struct Definition *def)
{
    if (lookup_var(current, def->name) != NULL) {
        printf("Error: variable '%s' shadows existing definition\n", lookup_symbol(def->name));
        return false;
    }
    def->scope_offset = current->varcount;
    current->varcount++;
    linkedlist_append(current->definitions, def);
    return true;
}

static bool add_type(struct Scope *current, struct Definition *def)
{
    linkedlist_foreach(lnode, current->definitions->head) {
        struct Definition *lookup_def = lnode->value;
        if (lookup_def->name == def->name) {
            lookup_def->type = def->type;
            lookup_def->scope_offset = def->scope_offset;
            return true;
        }
    }
    if (current->parent == NULL) {
        return false;
    } else {
        return add_type(current->parent, def);
    }
}

static bool add_class(struct ClassTable *ct, struct Class *cls)
{
    uint64_t loc;
    find_open_loc(loc, ct->table, ct->size, cls->name);
    struct Class *clst = &ct->table[loc];
    clst->name = cls->name;
    clst->definitions = cls->definitions;
    return true;
    // Eventually we'll need to figure out what to do with methods... maybe a table for each?
}

static bool add_function(struct FunctionTable *ft, struct FunDef *fundef)
{
    uint64_t loc = fundef->name % ft->size;
    find_open_loc(loc, ft->table, ft->size, fundef->name);
    struct FunDef *new_fundef = &ft->table[loc];
    new_fundef->name = fundef->name;
    // new_fundef->rettype = TYPE_UNDEFINED;
    new_fundef->rettype = fundef->rettype;
    new_fundef->args = fundef->args;
    new_fundef->stmts = fundef->stmts;
    return true;
}

// Add declared types to ast without typechecking
// TODO: Validate class / function definitions
// Basically just make sure that there aren't 2 properties / arguments with the same name
static bool add_types(TopLevelDecls *tlds, struct ClassTable *clst, struct FunctionTable *ft)
{
    linkedlist_foreach(lnode, tlds->head) {
        struct TopLevelDecl *tld = lnode->value;
        if (tld->type == TLD_TYPE_FUNDEF) {
            add_function(ft, tld->fundef);
        } else {
            add_class(clst, tld->cls);
        }
    }
    return true;
}

static Type typecheck_value(struct TypeCheckerContext *context, struct Value *val);

static Type typecheck_expression(struct TypeCheckerContext *context, struct Expr *expr)
{
    const Type type_left = typecheck_value(context, expr->val1);
    expr->val1->type = type_left;
    const Type type_right = (expr->val2 == NULL)
        ? TYPE_UNDEFINED
        : typecheck_value(context, expr->val2);
    if (expr->val2 != NULL) {
        expr->val2->type = type_right;
    }
    const bool left_is_int     = type_left  == TYPE_INT;
    const bool right_is_int    = type_right == TYPE_INT;
    const bool left_is_float   = type_left  == TYPE_FLOAT;
    const bool right_is_float  = type_right == TYPE_FLOAT;
    const bool right_is_null   = type_right == TYPE_UNDEFINED;
    const bool are_both_int    = left_is_int   && right_is_int;
    const bool are_both_float  = left_is_float && right_is_float;
    const bool are_both_bool   = (type_left  == TYPE_BOOL) && (type_right == TYPE_BOOL);
    const bool are_both_string = (type_left  == TYPE_STRING) && (type_right == TYPE_STRING);
    // const int are_both_other  = expr->val1 right_is_string;
    // have to find a way to handle non-primitives
    char *op_str = NULL;
    switch (expr->op) {
        case OP_OR:
            op_str = "||";
            // fall through
        case OP_AND:
            if (op_str == NULL) op_str = "&&";
            if (are_both_bool) {
                return TYPE_BOOL;
            } else {
                printf("Error: '%s' has non-boolean operands\n", op_str);
                return TYPE_UNDEFINED;
            }
        case OP_EQEQ:
            op_str = "==";
            // fall through
        case OP_NOT_EQ:
            if (op_str == NULL) op_str = "!=";
            if (are_both_int || are_both_float || are_both_bool || are_both_string) {
                return TYPE_BOOL;
            } else {
                printf("Error: mismatched types on '%s' operands\n", op_str);
                return TYPE_UNDEFINED;
            }
        case OP_GREATER_EQ:
            op_str = ">=";
            // fall through
        case OP_GREATER:
            if (op_str == NULL) op_str = ">";
            // fall through
        case OP_LESS_EQ:
            if (op_str == NULL) op_str = "<=";
            // fall through
        case OP_LESS:
            if (op_str == NULL) op_str = "<";
            if (are_both_int || are_both_float) {
                return TYPE_BOOL;
            } else {
                printf("Error: mismatched types on '%s' operands\n", op_str);
                return TYPE_UNDEFINED;
            }
        case OP_PLUS:
            if (are_both_int) {
                return TYPE_INT;
            } else if (are_both_float) {
                return TYPE_FLOAT;
            } else if (are_both_string) {
                return TYPE_STRING;
            } else {
                printf("Error: mismatched types for '+' operands\n");
                return TYPE_UNDEFINED;
            }
        case OP_MINUS:
            op_str = "-";
            // fall through
        case OP_STAR:
            if (op_str == NULL) op_str = "*";
            // fall through
        case OP_SLASH:
            if (op_str == NULL) op_str = "/";
            if (are_both_int) {
                return TYPE_INT;
            } else if (are_both_float) {
                return TYPE_FLOAT;
            } else {
                printf("Error: mismatched types for '%s' operands\n", op_str);
                return TYPE_UNDEFINED;
            }
        case OP_PERCENT:
            if (op_str == NULL) op_str = "%";
            if (are_both_int) {
                return TYPE_INT;
            } else if (are_both_float) {
                printf("Error: cannot use %s on floats\n", op_str);
                return TYPE_UNDEFINED;
            } else {
                printf("Error: mismatched types for '%s' operands\n", op_str);
                return TYPE_UNDEFINED;
            }
        case OP_UNARY_PLUS:
            op_str = "+";
            // fall through
        case OP_UNARY_MINUS:
            if (op_str == NULL) op_str = "-";
            if (left_is_int && right_is_null) {
                return TYPE_INT;
            } else if (left_is_float && right_is_null) {
                return TYPE_FLOAT;
            } else {
                printf("Error: expecting numeric operand for '%s'\n", op_str);
                return TYPE_UNDEFINED;
            }
        default:
            assert("Error, not implemented" && false);
            break;
    };
}

static Type typecheck_read(Values *args)
{
    if (args == NULL || args->length == 0) {
        return TYPE_STRING;
    }
    printf("Error: read function does not take arguments\n");
    return TYPE_UNDEFINED;
}

// static Type typecheck_symbol(struct TypeCheckerContext *context, Symbol symbol)
// {
//     struct Definition *def = lookup_var(context->scope, symbol);
//     if (def == NULL || def->type == TYPE_UNDEFINED) {
//         printf("Error: '%s' is not defined\n", lookup_symbol(symbol));
//         return TYPE_UNDEFINED;
//     } else {
//         return def->type;
//     }
// }

static Type typecheck_get(struct TypeCheckerContext *context, struct Accessor *get);

static Type typecheck_value(struct TypeCheckerContext *context, struct Value *val)
{
    switch (val->vtype) {
        case VTYPE_STRING:
            return TYPE_STRING;
        case VTYPE_INT:
            return TYPE_INT;
        case VTYPE_FLOAT:
            return TYPE_FLOAT;
        case VTYPE_BOOL:
            return TYPE_BOOL;
        case VTYPE_EXPR:
            val->type = typecheck_expression(context, val->expr);
            return val->type;
        case VTYPE_CONSTRUCTOR:
            val->type = typecheck_constructor(context, val->constructor);
            return val->type;
        case VTYPE_FUNCALL: {
            struct FunDef *fundef = lookup_fun(context->fun_table, val->funcall->funname);
            if (fundef != NULL && fundef->rettype == TYPE_UNDEFINED) {
                printf("Error: function %s does not return a value\n", lookup_symbol(fundef->name));
                return TYPE_UNDEFINED;
            } else if (typecheck_funcall(context, val->funcall, fundef)) {
                val->type = fundef->rettype;
                return fundef->rettype;
            } else {
                return TYPE_UNDEFINED;
            }
        }
        case VTYPE_GET:
            val->type = typecheck_get(context, val->get);
            return val->type;
        case VTYPE_BUILTIN_FUNCALL:
            Symbol name = val->funcall->funname;
            if (name == BUILTIN_READ) {
                val->type = TYPE_STRING;
                return typecheck_read(val->funcall->args);
            } else {
                assert("Error: not implemented\n" && false);
            }
        case VTYPE_BUILTIN_CONSTRUCTOR:
            assert("Error: not implemented\n" && false);
    }
    return TYPE_UNDEFINED; // Doing this to silence compiler warning, should never happen
}

// Prints first n lvalues
void print_lhs(Symbol symbol, LValues *lvalues, int n)
{
    printf("%s", lookup_symbol(symbol));
    int i = 0;
    linkedlist_foreach(lnode, lvalues->head) {
        if (i > n) {
            return;
        }
        struct LValue *lvalue = lnode->value;
        switch (lvalue->lvtype) {
            case LV_PROPERTY:
                printf(".");
                printf("%s", lookup_symbol(lvalue->property));
                break;
            case LV_INDEX:
                printf("[");
                print_value(lvalue->index);
                printf("]");
                break;
            default:
                assert("Error, not implemented" && false);
                break;
        }
        i++;
    }
}

// TODO: finish making this work
static Type typecheck_lvalue(struct TypeCheckerContext *context, struct Definition *def,
        LValues *lvalues)
{
    if (!is_object(def->type)) {
        struct LValue *lvalue = lvalues->head->value;
        printf("Error: '%s' is not an object and does not have property '%s'\n",
                lookup_symbol(def->name), lookup_symbol(lvalue->property));
        return TYPE_UNDEFINED;
    }
    linkedlist_foreach(lnode, lvalues->head) {
        struct LValue *lvalue = lnode->value;
        struct Class *cls = NULL;
        switch (lvalue->type) {
            case LV_PROPERTY:
                cls = lookup_class(context->cls_table, def->type);
                if (cls == NULL) {
                    printf("Error: '%s' is not an object and does not have property '%s'\n",
                            lookup_symbol(def->name), lookup_symbol(lvalue->property));
                    return TYPE_UNDEFINED;
                }
                Type old_type = def->type;
                def = lookup_property(cls, lvalue->property);
                if (def == NULL) {
                    printf("Error: %s instance has no property called '%s'\n",
                            lookup_symbol(old_type), lookup_symbol(lvalue->property));
                    return TYPE_UNDEFINED;
                }
                lvalue->type = def->type;
                break;
            case LV_INDEX:
                printf("Error, not implemented: typechecking for array indexing\n");
                return TYPE_UNDEFINED;
        }
    }
    return def->type;
}

static bool typecheck_set(struct TypeCheckerContext *context, struct Set *set)
{
    Type lhs_type, rhs_type;
    // If this is a combo definition + set, add definition to scope
    if (set->is_define) {
        assert(linkedlist_is_empty(set->to_set->lvalues)); // Not syntactically valid, should never happen
        struct Definition *def = allocator_malloc(sizeof(*def));
        def->name = set->to_set->definition->name;
        def->type = TYPE_UNDEFINED;
        if (!add_var(context->scope, def)) {
            return false;
        }
    }
    // Make sure left side has been declared
    struct Definition *def = lookup_var(context->scope, set->to_set->definition->name);
    if (def == NULL) {
        printf("Error: Symbol '%s' is used before it is declared\n", lookup_symbol(set->to_set->definition->name));
        return false;
    }
    // Typecheck left side of set
    if (!linkedlist_is_empty(set->to_set->lvalues)) {
        lhs_type = typecheck_lvalue(context, def, set->to_set->lvalues);
        if (lhs_type == TYPE_UNDEFINED) {
            return false;
        }
    } else {
        lhs_type = def->type;
    }
    rhs_type = typecheck_value(context, set->val);
    if (rhs_type == TYPE_UNDEFINED) {
        return false;
    } else if (rhs_type != lhs_type && lhs_type != TYPE_UNDEFINED) {
        // Need to rewrite this slightly to account for lhs possibly having properties
        printf("Error: %s is of type %s, cannot set to type %s\n",
                lookup_symbol(set->to_set->definition->name),
                lookup_symbol(lhs_type),
                lookup_symbol(rhs_type));
        return false;
    }
    if (linkedlist_is_empty(set->to_set->lvalues)) {
        def->type = rhs_type;
        add_type(context->scope, def);
    }
    set->to_set->definition->type = def->type;
    set->to_set->definition->scope_offset = def->scope_offset;
    return true;
}

// struct Accessor {
//     Symbol symbol;
//     Type type;
//     LValues *lvalues;
// };
static Type typecheck_get(struct TypeCheckerContext *context, struct Accessor *get)
{
    struct Definition *def = lookup_var(context->scope, get->definition->name);
    if (def == NULL) {
        printf("Error: Symbol '%s' is used before it is declared\n", lookup_symbol(get->definition->name));
        return TYPE_UNDEFINED;
    }
    Type type = !linkedlist_is_empty(get->lvalues)
        ? typecheck_lvalue(context, def, get->lvalues)
        : def->type;
    get->definition->type = def->type;
    get->definition->scope_offset = def->scope_offset;
    return type;
}

static bool scope_vars(struct Scope *scope, Definitions *defs)
{
    linkedlist_foreach(lnode, defs->head) {
        struct Definition *def = lnode->value;
        if (!add_var(scope, def)) {
            return false;
        }
    }
    return true;
}

static bool typecheck_if(struct TypeCheckerContext *context, struct IfStatement *if_stmt)
{
    Type t0 = typecheck_value(context, if_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return false;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in if condition\n");
        return false;
    }
    bool ret = typecheck_statements(context, if_stmt->if_stmts);
    if (if_stmt->else_stmts != NULL) {
        ret = ret && typecheck_statements(context, if_stmt->else_stmts);
    }
    return ret;
}

static bool typecheck_while(struct TypeCheckerContext *context, struct While *while_stmt)
{
    Type t0 = typecheck_value(context, while_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return false;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in while condition\n");
        return false;
    }
    return typecheck_statements(context, while_stmt->stmts);
}

static bool typecheck_for(struct TypeCheckerContext *context, struct For *for_stmt)
{
    bool ret = typecheck_statement(context, for_stmt->init);
    if (!ret) return false;
    Type t0 = typecheck_value(context, for_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return false;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean as second parameter in for loop condition\n");
        return false;
    }
    ret = typecheck_statement(context, for_stmt->increment);
    if (!ret) return false;
    return typecheck_statements(context, for_stmt->stmts);
}

static bool typecheck_return(struct TypeCheckerContext *context, struct Value *val)
{
    Type rettype = context->enclosing_func->rettype;
    if (val == NULL) {
        if (rettype == TYPE_UNDEFINED) {
            return true;
        } else {
            printf("Error: function should return %s but is returning nothing\n",
                   lookup_symbol(rettype));
            return false;
        }
    }
    Type val_type = typecheck_value(context, val);
    if (val_type == TYPE_UNDEFINED) {
        return false;
    } else if (rettype != val_type) {
        printf("Error: function return type is %s but returning value of type %s\n",
               lookup_symbol(rettype), lookup_symbol(val_type));
        return false;
    }
    return true;
}

static bool typecheck_funcall(struct TypeCheckerContext *context, struct FunCall *funcall,
                              struct FunDef *fundef)
{
    // struct FunDef *fundef = lookup_fun(context->fun_table, funcall->funname);
    if (fundef == NULL) {
        printf("Error: no function named %s\n", lookup_symbol(funcall->funname));
        return false;
    } 

    uint64_t fundef_arg_count  = fundef->args  == NULL ? 0 : fundef->args->length;
    uint64_t funcall_arg_count = funcall->args == NULL ? 0 : funcall->args->length;
    if (fundef_arg_count == 0 && funcall_arg_count == 0) {
        return true;
    } else if (fundef_arg_count != funcall_arg_count) {
        printf("Error: %s expects %" PRIu64 " arguments but got %" PRIu64 "\n",
                lookup_symbol(fundef->name),
                fundef_arg_count, funcall_arg_count);
        return false;
    }

    // Validate arguments
    struct LinkedListNode *vals = funcall->args->head;
    struct LinkedListNode *defs = fundef->args->head;
    for (uint64_t i = 0; i < funcall->args->length; i++) {
        struct Value *val = vals->value;
        struct Definition *fun_arg_def = defs->value;
        Type val_type = typecheck_value(context, val);
        if (val_type == TYPE_UNDEFINED) {
            return false;
        } else if (val_type != fun_arg_def->type) {
            printf("Error: expected argument %" PRIu64 " to %s to be of type %s, but got "
                   "argument of type %s\n",
                    i + 1, lookup_symbol(fundef->name),
                    lookup_symbol(fun_arg_def->type),
                    lookup_symbol(val_type));
            return false;
        }
        vals = vals->next;
        defs = defs->next;
    }
    return true;
}

// Typecheck for print functions
static bool typecheck_print(struct TypeCheckerContext *context, Values *args)
{
    if (args == NULL || args->length == 0) {
        printf("Error: print function requires at least one argument\n");
        return false;
    }
    // Validate arguments
    linkedlist_foreach(lnode, args->head) {
        struct Value *val = lnode->value;
        Type val_type = typecheck_value(context, val);
        if (val_type == TYPE_UNDEFINED) {
            return false;
        }
    }
    return true;
}

// struct Class {
//     Symbol name; // Name is same as type
//     Definitions *definitions;
//     Methods *methods;
// };
// TODO: make this handle non-trivial constructors
static Type typecheck_constructor(struct TypeCheckerContext *context, struct FunCall *constructor)
{
    struct Class *cls = lookup_class(context->cls_table, constructor->funname);
    if (cls == NULL) {
        printf("Error: no class named %s\n", lookup_symbol(constructor->funname));
        return TYPE_UNDEFINED;
    }

    uint64_t class_def_count       = cls->definitions  == NULL ? 0 : cls->definitions->length;
    uint64_t constructor_arg_count = constructor->args == NULL ? 0 : constructor->args->length;
    if (class_def_count == 0 && constructor_arg_count == 0) {
        return cls->name;
    } else if (class_def_count != constructor_arg_count) {
        printf("Error: %s expects %" PRIu64 " arguments but got %" PRIu64 "\n",
                lookup_symbol(cls->name),
                class_def_count, constructor_arg_count);
        return TYPE_UNDEFINED;
    }

    // Validate arguments
    struct LinkedListNode *vals = constructor->args->head;
    struct LinkedListNode *defs = cls->definitions->head;
    for (uint64_t i = 0; i < constructor->args->length; i++) {
        struct Value *val = vals->value;
        struct Definition *fun_arg_def = defs->value;
        Type val_type = typecheck_value(context, val);
        if (val_type == TYPE_UNDEFINED) {
            return false;
        } else if (val_type != fun_arg_def->type) {
            printf("Error: expected argument %" PRIu64 " to %s to be of type %s, but got "
                    "argument of type %s\n",
                    i + 1, lookup_symbol(cls->name),
                    lookup_symbol(fun_arg_def->type),
                    lookup_symbol(val_type));
            return TYPE_UNDEFINED;
        }
        vals = vals->next;
        defs = defs->next;
    }
    return cls->name;
}

static bool typecheck_statement(struct TypeCheckerContext *context, struct Statement *stmt)
{
    bool ret = false;
    switch (stmt->type) {
        case STMT_SET:
            return typecheck_set(context, stmt->set);
        case STMT_LET:
            return scope_vars(context->scope, stmt->let);
        case STMT_IF: {
            enter_scope(&(context->scope), false);
            ret = typecheck_if(context, stmt->if_stmt);
            exit_scope(&(context->scope));
            return ret;
        }
        case STMT_WHILE:
            enter_scope(&(context->scope), false);
            ret = typecheck_while(context, stmt->while_stmt);
            exit_scope(&(context->scope));
            return ret;
        case STMT_FOR:
            enter_scope(&(context->scope), false);
            ret = typecheck_for(context, stmt->for_stmt);
            exit_scope(&(context->scope));
            return ret;
        case STMT_BUILTIN_FUNCALL: {
            Symbol name = stmt->funcall->funname;
            if (name == BUILTIN_PRINT || name == BUILTIN_PRINTLN) {
                ret = typecheck_print(context, stmt->funcall->args);
            } else {
                assert("Error: not implemented\n" && false);
            }
            return ret;
        }
        case STMT_FUNCALL: {
            struct FunDef *fundef = lookup_fun(context->fun_table, stmt->funcall->funname);
            ret = typecheck_funcall(context, stmt->funcall, fundef);
            return ret;
            // return typecheck_funcall(context, stmt->funcall) != TYPE_UNDEFINED; 
        }
        case STMT_RETURN:
            return typecheck_return(context, stmt->ret);
        case STMT_FOREACH:
            assert("Error: not implemented\n" && false);
            enter_scope(&(context->scope), false);
            // ret = typecheck_foreach(context, stmt->foreach_stmt);
            exit_scope(&(context->scope));
            return ret;
        default:
            assert("Error, not implemented" && false);
            break;
    }
}

static bool typecheck_statements(struct TypeCheckerContext *context, Statements *stmts)
{
    linkedlist_foreach(lnode, stmts->head) {
        struct Statement *stmt = lnode->value;
        if (!typecheck_statement(context, stmt)) {
            return false;
        }
    }
    return true;
}

static bool typecheck_fundef(struct TypeCheckerContext *context, struct FunDef *fundef)
{
    struct Scope *scope = NULL;
    enter_scope(&scope, true);
    context->enclosing_func = fundef;
    context->scope = scope;
    bool ret = scope_vars(context->scope, fundef->args);
    if (!ret) {
        exit_scope(&scope);
        return ret;
    }
    ret = typecheck_statements(context, fundef->stmts);
    exit_scope(&scope);
    return ret;
}

static bool typecheck_tld(struct TypeCheckerContext *context, struct TopLevelDecl *tld)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            // typecheck_class(context, tld->cls);
            assert("Error: not implemented\n" && 1);
            return true;
        case TLD_TYPE_FUNDEF:
            if (!typecheck_fundef(context, tld->fundef)) {
                return false;
            }
            break;
    }
    return true;
}

static bool typecheck_tlds(struct TypeCheckerContext *context, TopLevelDecls *tlds)
{
    linkedlist_foreach(lnode, tlds->head) {
        if (!typecheck_tld(context, lnode->value)) {
            return false;
        }
    }
    return true;
}

bool typecheck(struct ClassTable *class_table,
        struct FunctionTable *function_table)
{
    if (!add_types(globals.ast, class_table, function_table)) {
        return false;
    }
    struct TypeCheckerContext context = { NULL, function_table, class_table, NULL };
    return typecheck_tlds(&context, globals.ast);
}

#undef find_open_loc

