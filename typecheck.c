#include "printers.h"
#include "typecheck.h"
#include "ast.h"

#define ALL_GOOD 1
#define BAD_STUFF 0

#define find_open_loc(i, table, length, symbol)\
    for (i = symbol % length; table[i].name != 0; i = i == length - 1 ? 0 : i + 1)

struct Scope {
    Definitions *definitions;
    struct Scope *parent;
};

struct TypeCheckerContext {
    struct FunDef *enclosing_func;
    struct FunctionTable *fun_table;
    struct ClassTable *cls_table;
    struct Scope *scope;
};

static int typecheck_statements(struct TypeCheckerContext *context, Statements *stmts);
static int typecheck_statement(struct TypeCheckerContext *context, struct Statement *stmt);
static Type typecheck_funcall(struct TypeCheckerContext *context, struct FunCall *funcall);
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
    printf("<- { ");
    for (Definitions *defs = scope->definitions; defs != NULL; defs = defs->next) {
        struct Definition *def = defs->value;
        printf("%s: %s", lookup_symbol(def->name), lookup_symbol(def->type));
        if (defs->next != NULL) {
            printf(", ");
        }
    }
    printf(" }\n");
}

static void enter_scope(struct Scope **current)
{
    struct Scope *scope = malloc(sizeof(*scope));
    scope->definitions = NULL;
    scope->parent = *current;
    *current = scope;
}

static void exit_scope(struct Scope **current)
{
    *current = (*current)->parent;
}

static void add_var(struct Scope *current, struct Definition *def)
{
    if (current->definitions == NULL) {
        current->definitions = new_list(def);
    } else {
        current->definitions = append(current->definitions, def);
    }
}

static int add_type(struct Scope *current, struct Definition *def)
{
    for (Definitions *defs = current->definitions; defs != NULL; defs = defs->next) {
        struct Definition *lookup_def = defs->value;
        if (lookup_def->name == def->name) {
            lookup_def->type = def->type;
            return 1;
        }
    }
    if (current->parent == NULL) {
        return 0;
    } else {
        return add_type(current->parent, def);
    }
}

static struct Definition *lookup_var(struct Scope *scope, Symbol name)
{
    for (Definitions *defs = scope->definitions; defs != NULL; defs = defs->next) {
        struct Definition *def = defs->value;
        if (def->name == name) {
            return def;
        }
    }
    if (scope->parent != NULL) {
        return lookup_var(scope->parent, name);
    }
    return NULL;
}

static int add_class(struct ClassTable *ct, struct Class *cls)
{
    uint64_t loc;
    find_open_loc(loc, ct->table, ct->size, cls->name);
    struct Class *clst = &ct->table[loc];
    clst->name = cls->name;
    clst->definitions = cls->definitions;
    return 1;
    // Eventually we'll need to figure out what to do with methods... maybe a table for each?
}

static int add_function(struct FunctionTable *ft, struct FunDef *fundef)
{
    uint64_t loc;
    find_open_loc(loc, ft->table, ft->size, fundef->name);
    struct FunDef *new_fundef = &ft->table[loc];
    new_fundef->name = fundef->name;
    // new_fundef->rettype = TYPE_UNDEFINED;
    new_fundef->rettype = fundef->rettype;
    new_fundef->args = fundef->args;
    new_fundef->stmts = fundef->stmts;
    return 1;
}

// Add declared types to ast without typechecking
// TODO: Validate class / function definitions
// Basically just make sure that there aren't 2 properties / arguments with the same name
static int add_types(TopLevelDecls *tlds, struct ClassTable *clst, struct FunctionTable *ft)
{
    for (; tlds != NULL; tlds = tlds->next) {
        struct TopLevelDecl *tld = tlds->value;
        if (tld->type == TLD_TYPE_FUNDEF) {
            add_function(ft, tld->fundef);
        } else {
            add_class(clst, tld->cls);
        }
    }
    return 1;
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
    const int left_is_int     = type_left  == TYPE_INT;
    const int right_is_int    = type_right == TYPE_INT;
    const int left_is_float   = type_left  == TYPE_FLOAT;
    const int right_is_float  = type_right == TYPE_FLOAT;
    const int right_is_null   = type_right == TYPE_UNDEFINED;
    const int are_both_int    = left_is_int   && right_is_int;
    const int are_both_float  = left_is_float && right_is_float;
    const int are_both_bool   = (type_left  == TYPE_BOOL) && (type_right == TYPE_BOOL);
    const int are_both_string = (type_left  == TYPE_STRING) && (type_right == TYPE_STRING);
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
    };
}

static Type typecheck_symbol(struct TypeCheckerContext *context, Symbol symbol)
{
    struct Definition *def = lookup_var(context->scope, symbol);
    if (def == NULL || def->type == TYPE_UNDEFINED) {
        printf("Error: '%s' is not defined\n", lookup_symbol(symbol));
        return TYPE_UNDEFINED;
    } else {
        return def->type;
    }
}

static Type typecheck_value(struct TypeCheckerContext *context, struct Value *val)
{
    switch (val->vtype) {
        case VTYPE_STRING:      return TYPE_STRING;
        case VTYPE_INT:         return TYPE_INT;
        case VTYPE_FLOAT:       return TYPE_FLOAT;
        case VTYPE_BOOL:        return TYPE_BOOL;
        case VTYPE_SYMBOL:      return typecheck_symbol(context, val->symbol);
        case VTYPE_EXPR:        return typecheck_expression(context, val->expr);
        case VTYPE_FUNCALL:     return typecheck_funcall(context, val->funcall);
        case VTYPE_CONSTRUCTOR: return typecheck_constructor(context, val->funcall);
    }
}

// Prints first n lvalues
static void print_lhs(Symbol symbol, LValues *lvalues, int n)
{
    printf("%s", lookup_symbol(symbol));
    int i = 0;
    for (; lvalues != NULL; lvalues = lvalues->next) {
        if (i > n) {
            return;
        }
        struct LValue *lvalue = lvalues->value;
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
        }
        i++;
    }
}

// TODO: finish making this work
static Type typecheck_lvalue(struct TypeCheckerContext *context, struct Definition *def,
        LValues *lvalues)
{
    if (!is_object(def->type)) {
        struct LValue *lvalue = lvalues->value;
        printf("Error: '%s' is not an object and does not have property '%s'\n",
                lookup_symbol(def->name), lookup_symbol(lvalue->property));
        return TYPE_UNDEFINED;
    }
    for (; lvalues != NULL; lvalues = lvalues->next) {
        struct LValue *lvalue = lvalues->value;
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

static int typecheck_set(struct TypeCheckerContext *context, struct Set *set)
{
    Type lhs_type, rhs_type;
    // If this is a combo definition + set, add definition to scope
    if (set->is_define) {
        assert(set->lvalues == NULL); // Not syntactically valid, should never happen
        struct Definition *def = malloc(sizeof(*def));
        def->name = set->symbol;
        def->type = TYPE_UNDEFINED;
        add_var(context->scope, def);
    }
    // Make sure left side has been declared
    struct Definition *def = lookup_var(context->scope, set->symbol);
    if (def == NULL) {
        printf("Error: Symbol '%s' is used before it is declared\n", lookup_symbol(set->symbol));
        return 0;
    }
    // Typecheck left side of set
    if (set->lvalues != NULL) {
        lhs_type = typecheck_lvalue(context, def, set->lvalues);
        if (lhs_type == TYPE_UNDEFINED) {
            return 0;
        }
    } else {
        lhs_type = def->type;
    }
    rhs_type = typecheck_value(context, set->val);
    if (rhs_type == TYPE_UNDEFINED) {
        return 0;
    } else if (rhs_type != lhs_type && lhs_type != TYPE_UNDEFINED) {
        // Need to rewrite this slightly to account for lhs possibly having properties
        printf("Error: %s is of type %s, cannot set to type %s\n",
                lookup_symbol(set->symbol),
                lookup_symbol(lhs_type),
                lookup_symbol(rhs_type));
        return 0;
    }
    if (set->lvalues == NULL) {
        def->type = rhs_type;
        add_type(context->scope, def);
    }
    set->type = def->type;
    return 1;
}

static void scope_vars(struct Scope *scope, Definitions *defs)
{
    for (; defs != NULL; defs = defs->next) {
        struct Definition *def = defs->value;
        add_var(scope, def);
    }
}

static int typecheck_if(struct TypeCheckerContext *context, struct IfStatement *if_stmt)
{
    enter_scope(&context->scope);
    Type t0 = typecheck_value(context, if_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in if condition\n");
        return 0;
    }
    int ret = typecheck_statements(context, if_stmt->if_stmts);
    if (if_stmt->else_stmts != NULL) {
        ret = ret && typecheck_statements(context, if_stmt->else_stmts);
    }
    exit_scope(&context->scope);
    return ret;
}

static int typecheck_while(struct TypeCheckerContext *context, struct While *while_stmt)
{
    Type t0 = typecheck_value(context, while_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in while condition\n");
        return 0;
    }
    int ret = typecheck_statements(context, while_stmt->stmts);
    return ret;
}

static int typecheck_for(struct TypeCheckerContext *context, struct For *for_stmt)
{
    int ret = typecheck_statement(context, for_stmt->init);
    if (!ret) return 0;
    Type t0 = typecheck_value(context, for_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean as second parameter as for loop condition\n");
        return 0;
    }
    ret = typecheck_statement(context, for_stmt->increment);
    if (!ret) return 0;
    ret = typecheck_statements(context, for_stmt->stmts);
    return ret;
}

static int typecheck_return(struct TypeCheckerContext *context, struct Value *val)
{
    char *errmsg = "Error: function return type is %s but returning value of type %s\n";
    Type rettype = context->enclosing_func->rettype;
    if (val == NULL) {
        if (rettype == TYPE_UNDEFINED) {
            return 1;
        } else {
            printf(errmsg, "void", lookup_symbol(rettype));
            return 0;
        }
    }
    Type val_type = typecheck_value(context, val);
    if (val_type == TYPE_UNDEFINED) {
        return 0;
    } else if (rettype != val_type) {
        printf(errmsg, lookup_symbol(rettype), lookup_symbol(val_type));
        return 0;
    }
    return 1;
}

static Type typecheck_funcall(struct TypeCheckerContext *context, struct FunCall *funcall)
{
    struct FunDef *fundef = lookup_fun(context->fun_table, funcall->funname);
    if (fundef == NULL) {
        printf("Error: no function named %s\n", lookup_symbol(funcall->funname));
        return TYPE_UNDEFINED;
    } 

    uint64_t fundef_arg_count  = fundef->args  == NULL ? 0 : fundef->args->length;
    uint64_t funcall_arg_count = funcall->args == NULL ? 0 : funcall->args->length;
    if (fundef_arg_count == 0 && funcall_arg_count == 0) {
        return 1;
    } else if (fundef_arg_count != funcall_arg_count) {
        printf("Error: %s expects %" PRIu64 " arguments but got %" PRIu64 "\n",
                lookup_symbol(fundef->name),
                fundef_arg_count, funcall_arg_count);
        return TYPE_UNDEFINED;
    }

    // Validate arguments
    Values *vals = funcall->args;
    Definitions *defs = fundef->args;
    for (uint64_t i = 0; i < funcall->args->length; i++) {
        struct Value *val = vals->value;
        struct Definition *fun_arg_def = defs->value;
        Type val_type = typecheck_value(context, val);
        if (val_type != fun_arg_def->type) {
            printf("Error: expected argument %" PRIu64 " to %s to be of type %s, but got "
                   "argument of type %s\n",
                    i + 1, lookup_symbol(fundef->name),
                    lookup_symbol(fun_arg_def->type),
                    lookup_symbol(val_type));
            return TYPE_UNDEFINED;
        }
        vals = vals->next;
        defs = defs->next;
    }
    return fundef->rettype;
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
        return 1;
    } else if (class_def_count != constructor_arg_count) {
        printf("Error: %s expects %" PRIu64 " arguments but got %" PRIu64 "\n",
                lookup_symbol(cls->name),
                class_def_count, constructor_arg_count);
        return TYPE_UNDEFINED;
    }

    // Validate arguments
    Values *vals = constructor->args;
    Definitions *defs = cls->definitions;
    for (uint64_t i = 0; i < constructor->args->length; i++) {
        struct Value *val = vals->value;
        struct Definition *fun_arg_def = defs->value;
        Type val_type = typecheck_value(context, val);
        if (val_type != fun_arg_def->type) {
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

static int typecheck_statement(struct TypeCheckerContext *context, struct Statement *stmt)
{
    switch (stmt->type) {
        case STMT_SET:     return typecheck_set(context, stmt->set);
        case STMT_LET:     scope_vars(context->scope, stmt->let); return 1;
        case STMT_IF:      return typecheck_if(context, stmt->if_stmt);
        case STMT_WHILE:   return typecheck_while(context, stmt->while_stmt);
        case STMT_FOR:     return typecheck_for(context, stmt->for_stmt);
        case STMT_FUNCALL: return typecheck_funcall(context, stmt->funcall) != TYPE_UNDEFINED;
        case STMT_RETURN:  return typecheck_return(context, stmt->ret);
        case STMT_FOREACH: printf("Error: not implemented\n");
                           return 0;
    }
}

static int typecheck_statements(struct TypeCheckerContext *context, Statements *stmts)
{
    for (; stmts != NULL; stmts = stmts->next) {
        struct Statement *stmt = stmts->value;
        if (!typecheck_statement(context, stmt)) {
            return 0;
        }
    }
    return 1;
}

static int typecheck_fundef(struct TypeCheckerContext *context, struct FunDef *fundef)
{
    struct Scope *scope = NULL;
    enter_scope(&scope);
    context->enclosing_func = fundef;
    context->scope = scope;
    scope_vars(context->scope, fundef->args);
    int ret = typecheck_statements(context, fundef->stmts);
    exit_scope(&scope);
    return ret;
}

static int typecheck_tld(struct TypeCheckerContext *context, struct TopLevelDecl *tld)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            // typecheck_class(context, tld->cls);
            // printf("Error: not implemented\n");
            return 1;
        case TLD_TYPE_FUNDEF:
            if (!typecheck_fundef(context, tld->fundef)) {
                return 0;
            }
            break;
    }
    return 1;
}

static int typecheck_tlds(struct TypeCheckerContext *context, TopLevelDecls *tlds)
{
    for (;tlds != NULL; tlds = tlds->next) {
        if (!typecheck_tld(context, tlds->value)) {
            return 0;
        }
    }
    return 1;
}

int typecheck(struct Ast *ast, struct ClassTable *class_table,
        struct FunctionTable *function_table)
{
    if (!add_types(ast->ast, class_table, function_table)) {
        return 0;
    }
    // for (uint64_t i = 0; i < ast->class_count; i++) {
    //     print_class(&(clst[i]), 0);
    //     printf("\n");
    // }
    // for (uint64_t i = 0; i < ast->function_count; i++) {
    //     print_fundef(&(ft[i]), 0, 0);
    //     printf("\n");
    // }
    struct TypeCheckerContext context = { NULL, function_table, class_table, NULL };
    return typecheck_tlds(&context, ast->ast);
}

#undef find_open_loc

