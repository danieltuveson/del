#include "common.h"
#include "allocator.h"
#include "linkedlist.h"
#include "ast.h"
#include "printers.h"
#include "typecheck.h"

#define find_open_loc(i, table, length, symbol)\
    for (i = symbol % length; table[i].name != 0; i = i == length - 1 ? 0 : i + 1)

struct TypeCheckerContext {
    bool has_entrypoint;
    struct FunDef *enclosing_func;
    struct FunctionTable *fun_table;
    struct ClassTable *cls_table;
    struct Scope *scope;
};

/* Misc forward declarations */
static bool typecheck_statements(struct Globals *globals, struct TypeCheckerContext *context,
        Statements *stmts);
static bool typecheck_statement(struct Globals *globals, struct TypeCheckerContext *context,
        struct Statement *stmt);
static bool typecheck_funcall(struct Globals *globals, struct TypeCheckerContext *context,
        struct FunCall *funcall, struct FunDef *fundef);
static Type typecheck_constructor(struct Globals *globals, struct TypeCheckerContext *context,
        struct Constructor *constructor);
static Type typecheck_get_local(struct Globals *globals, struct TypeCheckerContext *context,
        struct Definition *get_local);
static Type typecheck_get_property(struct Globals *globals, struct TypeCheckerContext *context,
        struct GetProperty *get);
static Type typecheck_get_index(struct Globals *globals, struct TypeCheckerContext *context,
        struct GetProperty *get);
static bool typecheck_type(struct Globals *globals, struct TypeCheckerContext *context, Type type);

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
    if (ct->size == 0) {
        return NULL;
    }
    table_lookup(ct->table, ct->size, symbol);
}

struct FunDef *lookup_fun(struct FunctionTable *ft, Symbol symbol)
{
    if (ft->size == 0) {
        return NULL;
    }
    table_lookup(ft->table, ft->size, symbol);
}

#undef table_lookup

static void enter_scope(struct Globals *globals, struct Scope **current, bool isfunction,
        bool isloop)
{
    struct Scope *scope = allocator_malloc(globals->allocator, sizeof(*scope));
    scope->isfunction = isfunction;
    scope->isloop = isloop;
    scope->rettype = TYPE_UNDEFINED;
    if (*current != NULL && !isfunction) {
        scope->varcount = (*current)->varcount;
        scope->objcount = (*current)->objcount;
    } else {
        scope->varcount = 0;
        scope->objcount = 0;
    }
    scope->definitions = linkedlist_new(globals->allocator);
    scope->parent = *current;
    *current = scope;
}

static void exit_scope(struct Scope **current)
{
    *current = (*current)->parent;
}

static struct Definition *lookup_current_scope_var(struct Scope *scope, Symbol name)
{
    linkedlist_foreach(lnode, scope->definitions->head) {
        struct Definition *def = lnode->value;
        if (def->name == name) {
            return def;
        }
    }
    return NULL;
}

static struct Definition *lookup_var(struct Scope *scope, Symbol name)
{
    struct Definition *def = lookup_current_scope_var(scope, name);
    if (def == NULL && scope->parent != NULL) {
        return lookup_var(scope->parent, name);
    }
    return def;
}

static bool is_in_loop(struct Scope *scope)
{
    if (scope->isloop) {
        return true;
    } else if (scope->parent != NULL) {
        return is_in_loop(scope->parent);
    }
    return false;
}

static bool add_var(struct Globals *globals, struct TypeCheckerContext *context,
        struct Definition *def)
{
    context->enclosing_func->num_locals++;
    if (lookup_current_scope_var(context->scope, def->name) != NULL) {
        fprintf(globals->ferr, "Error: variable '%s' shadows existing definition\n",
                lookup_symbol(globals, def->name));
        return false;
    }
    if (is_object(def->type)) {
        def->scope_offset = context->scope->objcount;
        context->scope->objcount++;
    } else {
        def->scope_offset = context->scope->varcount;
        context->scope->varcount++;
    }
    linkedlist_append(context->scope->definitions, def);
    return true;
}

// static bool add_type(struct Scope *current, struct Definition *def)
// {
//     linkedlist_foreach(lnode, current->definitions->head) {
//         struct Definition *lookup_def = lnode->value;
//         if (lookup_def->name == def->name) {
//             lookup_def->type = def->type;
//             lookup_def->scope_offset = def->scope_offset;
//             return true;
//         }
//     }
//     if (current->parent == NULL) {
//         return false;
//     } else {
//         return add_type(current->parent, def);
//     }
// }

static bool add_class(struct ClassTable *ct, struct Class *cls)
{
    uint64_t loc;
    find_open_loc(loc, ct->table, ct->size, cls->name);
    struct Class *clst = &ct->table[loc];
    memcpy(clst, cls, sizeof(*cls));
    return true;
    // Eventually we'll need to figure out what to do with methods... maybe a table for each?
}

static bool add_function(struct FunctionTable *ft, struct FunDef *fundef)
{
    uint64_t loc = fundef->name % ft->size;
    find_open_loc(loc, ft->table, ft->size, fundef->name);
    struct FunDef *new_fundef = &ft->table[loc];
    memcpy(new_fundef, fundef, sizeof(*fundef));
    return true;
}

static bool add_types(TopLevelDecls *tlds, struct ClassTable *clst, struct FunctionTable *ft)
{
    linkedlist_foreach(lnode, tlds->head) {
        struct TopLevelDecl *tld = lnode->value;
        if (tld->type == TLD_TYPE_CLASS) {
            add_class(clst, tld->cls);
        } else { //if (tld->type == TLD_TYPE_FUNDEF) {
            add_function(ft, tld->fundef);
        }
    }
    return true;
}

static Type typecheck_value(struct Globals *globals, struct TypeCheckerContext *context,
        struct Value *val);

static Type typecheck_expression(struct Globals *globals, struct TypeCheckerContext *context,
        struct Expr *expr)
{
    // Typecheck left
    Type type_left = typecheck_value(globals, context, expr->val1);
    if (type_left == TYPE_UNDEFINED) {
        return TYPE_UNDEFINED;
    }
    expr->val1->type = type_left;

    // Typecheck right (if applicable)
    Type type_right = TYPE_UNDEFINED;
    if (expr->val2 != NULL) {
        type_right = typecheck_value(globals, context, expr->val2);
        if (type_right == TYPE_UNDEFINED) {
            return TYPE_UNDEFINED;
        }
        expr->val2->type = type_right;
    }

    const bool left_is_int     = type_left  == TYPE_INT;
    const bool right_is_int    = type_right == TYPE_INT;
    const bool left_is_float   = type_left  == TYPE_FLOAT;
    const bool right_is_float  = type_right == TYPE_FLOAT;
    const bool right_is_undef  = type_right == TYPE_UNDEFINED;
    const bool are_both_int    = left_is_int   && right_is_int;
    const bool are_both_float  = left_is_float && right_is_float;
    const bool are_both_bool   = (type_left  == TYPE_BOOL) && (type_right == TYPE_BOOL);
    const bool are_both_string = (type_left  == TYPE_STRING) && (type_right == TYPE_STRING);
    const bool are_both_byte   = (type_left  == TYPE_BYTE) && (type_right == TYPE_BYTE);
    const bool are_both_obj    = is_object_or_null(type_left) && is_object_or_null(type_right);
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
            }
            break;
        case OP_EQEQ:
            op_str = "==";
            // fall through
        case OP_NOT_EQ:
            if (op_str == NULL) op_str = "!=";
            if (are_both_int || are_both_float || are_both_bool || are_both_string
                    || are_both_byte || are_both_obj) {
                return TYPE_BOOL;
            }
            break;
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
            if (are_both_int || are_both_float || are_both_byte) {
                return TYPE_BOOL;
            }
            break;
        case OP_PLUS:
            op_str = "+";
            // fall through
        case OP_MINUS:
            if (op_str == NULL) op_str = "-";
            // fall through
        case OP_STAR:
            if (op_str == NULL) op_str = "*";
            // fall through
        case OP_SLASH:
            if (op_str == NULL) op_str = "/";
            if (are_both_int) {
                return TYPE_INT;
            } else if (are_both_byte) {
                return TYPE_BYTE;
            } else if (are_both_float) {
                return TYPE_FLOAT;
            }
            break;
        case OP_PERCENT:
            if (op_str == NULL) op_str = "%";
            if (are_both_int) {
                return TYPE_INT;
            } else if (are_both_byte) {
                return TYPE_BYTE;
            }
            break;
        case OP_UNARY_PLUS:
            op_str = "+";
            // fall through
        case OP_UNARY_MINUS:
            if (op_str == NULL) op_str = "-";
            if (left_is_int && right_is_undef) {
                return TYPE_INT;
            } else if (left_is_float && right_is_undef) {
                return TYPE_FLOAT;
            }
            break;
        case OP_UNARY_NOT:
            op_str = "!";
            if (type_left == TYPE_BOOL && right_is_undef) {
                return TYPE_BOOL;
            }
            break;
    };
    if (type_left != type_right && type_right != TYPE_UNDEFINED) {
        fprintf(globals->ferr, "Error: mismatched types for '%s' operands\n", op_str);
    } else {
        // This message works for both unary and binary, since we only have left associative
        // operators
        fprintf(globals->ferr, "Error: cannot use '%s' operator on objects of type '%s'\n",
                op_str, lookup_symbol(globals, type_left));
    }
    return TYPE_UNDEFINED;
}

// TODO: properly handle null literals
static Type typecheck_array_literal(struct Globals *globals, struct TypeCheckerContext *context,
        Values *vals)
{
    struct Value *val = NULL;
    int count = 0;
    Type inferred = TYPE_UNDEFINED;
    Type type = TYPE_UNDEFINED;
    linkedlist_vforeach(val, vals) {
        type = typecheck_value(globals, context, val);
        if (type == TYPE_UNDEFINED) {
            return TYPE_UNDEFINED;
        }
        if (is_array(type)) {
            // TODO: get rid of this
            fprintf(globals->ferr, "Error: array may not contain other arrays\n");
            return TYPE_UNDEFINED;
        }
        if (count == 0) {
            inferred = type;
        } else if (inferred != type) {
            fprintf(globals->ferr, "Error: element 0 of array is of type '%s', "
                    "but element %d is of type '%s'\n",
                    lookup_symbol(globals, inferred),
                    count,
                    lookup_symbol(globals, type));
            return TYPE_UNDEFINED;
        }
        count++;
    }
    // Always expect parser to parse at least one element
    assert(type != TYPE_UNDEFINED);
    return array_of(type);
}

static Type typecheck_new_array(struct Globals *globals, struct TypeCheckerContext *context,
        struct Constructor *constructor)
{
    // Validate type parameter
    uint64_t constructor_types_count = constructor->types == NULL ? 0 : constructor->types->length;
    if (constructor_types_count != 1) {
        fprintf(globals->ferr, "Error: Array expects 1 type parameter but got %" PRIu64 "\n",
                constructor_types_count);
        return TYPE_UNDEFINED;
    }
    // Validate arguments
    struct FunCall *funcall = constructor->funcall;
    uint64_t constructor_arg_count = funcall->args == NULL ? 0 : funcall->args->length;
    if (constructor_arg_count != 1) {
        fprintf(globals->ferr, "Error: Array expects 1 argument but got %" PRIu64 "\n", constructor_arg_count);
        return TYPE_UNDEFINED;
    }
    struct Value *arg = funcall->args->head->value;
    Type arg_type = typecheck_value(globals, context, arg);
    if (arg_type == TYPE_UNDEFINED) {
        return TYPE_UNDEFINED;
    } else if (arg_type != TYPE_INT) {
        fprintf(globals->ferr, "Error: expected argument to be of type int, but got argument of type %s\n",
                lookup_symbol(globals, arg_type));
        return TYPE_UNDEFINED;
    }
    Type *type_ptr = constructor->types->head->value;
    return array_of(*type_ptr);
}

static bool oneof(Type t, Type t0, Type t1, Type t2)
{
    return t == t0 || t == t1 || t == t2;
}

static Type typecheck_cast(struct Globals *globals, struct TypeCheckerContext *context,
        struct Cast *cast)
{
    Type type = typecheck_value(globals, context, cast->value);
    if (type == TYPE_UNDEFINED) {
        return TYPE_UNDEFINED;
    } else if (!typecheck_type(globals, context, cast->type)) {
        return TYPE_UNDEFINED;
    } else if (type == cast->type) {
        char *type_str = lookup_symbol(globals, type);
        fprintf(globals->ferr, "Error: cast from %s to %s is redundant\n", type_str, type_str);
        return TYPE_UNDEFINED;
    }
    switch (type) {
        case TYPE_INT:
            if (!(oneof(cast->type, TYPE_BOOL, TYPE_FLOAT, TYPE_BYTE))) {
                goto general_error;
            }
            return cast->type;
        case TYPE_BOOL:
            if (!(oneof(cast->type, TYPE_INT, TYPE_FLOAT, TYPE_BYTE))) {
                goto general_error;
            }
            return cast->type;
        case TYPE_FLOAT:
            if (!(oneof(cast->type, TYPE_BOOL, TYPE_INT, TYPE_BYTE))) {
                goto general_error;
            }
            return cast->type;
        case TYPE_BYTE:
            if (!(oneof(cast->type, TYPE_BOOL, TYPE_FLOAT, TYPE_INT))) {
                goto general_error;
            }
            return cast->type;
        case TYPE_STRING:
            if (!(is_array(cast->type) && type_of_array(cast->type) == TYPE_BYTE)) {
                goto general_error;
            }
            return cast->type;
        case TYPE_NULL:
            fprintf(globals->ferr, "Error: cannot cast null\n");
            return TYPE_UNDEFINED;
    }
    fprintf(globals->ferr, "Error: cannot cast objects\n");
    return TYPE_UNDEFINED;
general_error:
    fprintf(globals->ferr,
            "Error: cannot cast %s to %s\n",
            lookup_symbol(globals, type),
            lookup_symbol(globals, cast->type));
    return TYPE_UNDEFINED;
}

// static Type typecheck_read(struct Globals *globals, Values *args)
// {
//     // if (args == NULL || args->length == 0) {
//     //     return TYPE_STRING;
//     // }
//     // fprintf(globals->ferr, "Error: read function does not take arguments\n");
//     // return TYPE_UNDEFINED;
// }

static Type typecheck_value(struct Globals *globals, struct TypeCheckerContext *context,
        struct Value *val)
{
    Symbol name;
    switch (val->vtype) {
        case VTYPE_STRING:
            return TYPE_STRING;
        case VTYPE_BYTE:
            return TYPE_BYTE;
        case VTYPE_INT:
            return TYPE_INT;
        case VTYPE_FLOAT:
            return TYPE_FLOAT;
        case VTYPE_BOOL:
            return TYPE_BOOL;
        case VTYPE_NULL:
            return TYPE_NULL;
        case VTYPE_EXPR:
            val->type = typecheck_expression(globals, context, val->expr);
            return val->type;
        case VTYPE_ARRAY_LITERAL:
            val->type = typecheck_array_literal(globals, context, val->array);
            return val->type;
        case VTYPE_CAST:
            val->type = typecheck_cast(globals, context, val->cast);
            return val->type;
        case VTYPE_CONSTRUCTOR:
            val->type = typecheck_constructor(globals, context, val->constructor);
            return val->type;
        case VTYPE_BUILTIN_FUNCALL:
        case VTYPE_FUNCALL: {
            Symbol funname = val->funcall->access->definition->name;
            assert(linkedlist_is_empty(val->funcall->access->lvalues));
            struct FunDef *fundef = lookup_fun(context->fun_table, funname);
            if (fundef != NULL && fundef->rettype == TYPE_UNDEFINED) {
                fprintf(globals->ferr, "Error: function %s does not return a value\n",
                        lookup_symbol(globals, fundef->name));
                return TYPE_UNDEFINED;
            } else if (typecheck_funcall(globals, context, val->funcall, fundef)) {
                val->type = fundef->rettype;
                return fundef->rettype;
            } else {
                return TYPE_UNDEFINED;
            }
        }
        case VTYPE_GET_LOCAL:
            val->type = typecheck_get_local(globals, context, val->get_local);
            return val->type;
        case VTYPE_GET_PROPERTY:
            val->type = typecheck_get_property(globals, context, val->get_property);
            return val->type;
        case VTYPE_INDEX:
            val->type = typecheck_get_index(globals, context, val->get_property);
            return val->type;
        // case VTYPE_BUILTIN_FUNCALL:
        //     name = val->funcall->access->definition->name;
        //     assert(linkedlist_is_empty(val->funcall->access->lvalues));
        //     if (name == BUILTIN_READ) {
        //         val->type = TYPE_STRING;
        //         return typecheck_read(globals, val->funcall->args);
        //     // } else if (name == BUILTIN_CONCAT) {
        //     //     assert("Error: not implemented\n" && false);
        //     //     // val->type = TYPE_ARRAY;
        //     //     return typecheck_concat(globals, val->funcall->args);
        //     } else {
        //         assert("Error: not implemented\n" && false);
        //     }
        case VTYPE_BUILTIN_CONSTRUCTOR:
            name = val->constructor->funcall->access->definition->name;
            assert(linkedlist_is_empty(val->constructor->funcall->access->lvalues));
            if (name == BUILTIN_ARRAY) {
                val->type = typecheck_new_array(globals, context, val->constructor);
                return val->type;
            } 
    }
    assert("Error: unhandled typecheck_value\n" && false);
    return TYPE_UNDEFINED; // Doing this to silence compiler warning, should never happen
}

static Type typecheck_get_local(struct Globals *globals, struct TypeCheckerContext *context,
        struct Definition *get_local)
{
    struct Definition *def = lookup_var(context->scope, get_local->name);
    if (def == NULL) {
        fprintf(globals->ferr, "Error: Symbol '%s' is used before it is declared\n",
                lookup_symbol(globals, get_local->name));
        return TYPE_UNDEFINED;
    }
    get_local->type = def->type;
    get_local->scope_offset = def->scope_offset;
    return def->type;
}

static bool typecheck_set_local(struct Globals *globals, struct TypeCheckerContext *context,
        struct SetLocal *set)
{
    if (!set->is_define) {
        struct Definition *def = lookup_var(context->scope, set->def->name);
        if (def == NULL) {
            fprintf(globals->ferr, "Error: Symbol '%s' is used before it is declared\n",
                    lookup_symbol(globals, set->def->name));
            return false;
        }
        set->def = def;
    }
    Type rhs_type = typecheck_value(globals, context, set->expr);
    if (rhs_type == TYPE_UNDEFINED) {
        return false;
    } else if (rhs_type != set->def->type && set->def->type != TYPE_UNDEFINED) {
        fprintf(globals->ferr, "Error: %s is of type %s, cannot set to type %s\n",
                lookup_symbol(globals, set->def->name),
                lookup_symbol(globals, set->def->type),
                lookup_symbol(globals, rhs_type));
        return false;
    }
    set->def->type = rhs_type;
    if (set->is_define && !add_var(globals, context, set->def)) {
        return false;
    }
    // add_type(context->scope, set->def);
    return true;
}

static Type typecheck_get_property(struct Globals *globals, struct TypeCheckerContext *context,
        struct GetProperty *get)
{
    Type type = typecheck_value(globals, context, get->accessor);
    if (type == TYPE_UNDEFINED) {
        return TYPE_UNDEFINED;
    }
    const char *generic_err = "Error: object of type '%s' has no property called '%s'\n";
    // TODO: Add array length as get-able property of array object
    // Also make typecheck_set_property give a better error if someone tries to set length
    if (is_array(type)) {
        if (get->property == BUILTIN_LENGTH) {
            return TYPE_INT;
        }
        fprintf(globals->ferr, generic_err,
                lookup_symbol(globals, type),
                lookup_symbol(globals, get->property));
        return TYPE_UNDEFINED;
    }
    struct Class *cls = lookup_class(context->cls_table, type);
    if (cls == NULL) {
        fprintf(globals->ferr, "Error: '%s' is not an object\n",
                lookup_symbol(globals, type));
        return TYPE_UNDEFINED;
    }
    struct Definition *def = lookup_property(cls, get->property);
    if (def == NULL) {
        fprintf(globals->ferr, generic_err,
                lookup_symbol(globals, type),
                lookup_symbol(globals, get->property));
        return TYPE_UNDEFINED;
    }
    return def->type;
}

// Will make this allow broader range of types in the future
static Type typecheck_get_index(struct Globals *globals, struct TypeCheckerContext *context,
        struct GetProperty *get)
{
    Type array_type = typecheck_value(globals, context, get->accessor);
    Type index_type = typecheck_value(globals, context, get->index);
    if (array_type == TYPE_UNDEFINED) {
        return TYPE_UNDEFINED;
    } else if (!is_array(array_type)) {
        fprintf(globals->ferr, "Error: '%s' is not an array\n", lookup_symbol(globals, array_type));
        return TYPE_UNDEFINED;
    } else if (index_type == TYPE_UNDEFINED) {
        return TYPE_UNDEFINED;
    } else if (index_type != TYPE_INT) {
        fprintf(globals->ferr, "Error: array access must use int, but got value of type '%s'\n",
                lookup_symbol(globals, index_type));
        return TYPE_UNDEFINED;
    }
    return type_of_array(array_type);
}

static bool typecheck_setter(struct Globals *globals, struct TypeCheckerContext *context,
        struct SetProperty *set, Type type_get)
{
    Type type_set = typecheck_value(globals, context, set->expr);
    if (type_get == TYPE_UNDEFINED || type_set == TYPE_UNDEFINED) {
        return false;
    } else if (type_get != type_set) {
        fprintf(globals->ferr, "Error: cannot set field of type %s to value of type %s\n",
                lookup_symbol(globals, type_get),
                lookup_symbol(globals, type_set));
        return false;
    }
    return true;
}

static bool typecheck_set_property(struct Globals *globals, struct TypeCheckerContext *context,
        struct SetProperty *set)
{
    Type type_get = typecheck_get_property(globals, context, set->access);
    return typecheck_setter(globals, context, set, type_get);
}

static bool typecheck_set_index(struct Globals *globals, struct TypeCheckerContext *context,
        struct SetProperty *set)
{
    Type type_get = typecheck_get_index(globals, context, set->access);
    return typecheck_setter(globals, context, set, type_get);
}
static bool scope_vars(struct Globals *globals, struct TypeCheckerContext *context,
        Definitions *defs)
{
    linkedlist_foreach(lnode, defs->head) {
        struct Definition *def = lnode->value;
        if (!add_var(globals, context, def)) {
            return false;
        }
    }
    return true;
}

static bool typecheck_if(struct Globals *globals, struct TypeCheckerContext *context,
        struct IfStatement *if_stmt)
{
    Type t0 = typecheck_value(globals, context, if_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return false;
    } else if (t0 != TYPE_BOOL) {
        fprintf(globals->ferr, "Error: expected boolean in if condition\n");
        return false;
    }
    enter_scope(globals, &(context->scope), false, false);
    bool ret = typecheck_statements(globals, context, if_stmt->if_stmts);
    Type rettype_if = context->scope->rettype;
    exit_scope(&(context->scope));
    if (if_stmt->else_stmts != NULL) {
        enter_scope(globals, &(context->scope), false, false);
        ret = ret && typecheck_statements(globals, context, if_stmt->else_stmts);
        Type rettype_else = context->scope->rettype;
        exit_scope(&(context->scope));
        if (rettype_if != TYPE_UNDEFINED && rettype_else != TYPE_UNDEFINED) {
            context->scope->rettype = rettype_else;
        }
    }
    return ret;
}

static bool typecheck_while(struct Globals *globals, struct TypeCheckerContext *context,
        struct While *while_stmt)
{
    Type t0 = typecheck_value(globals, context, while_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return false;
    } else if (t0 != TYPE_BOOL) {
        fprintf(globals->ferr, "Error: expected boolean in while condition\n");
        return false;
    }
    return typecheck_statements(globals, context, while_stmt->stmts);
}

static bool typecheck_for(struct Globals *globals, struct TypeCheckerContext *context,
        struct For *for_stmt)
{
    bool ret = typecheck_statement(globals, context, for_stmt->init);
    if (!ret) return false;
    Type t0 = typecheck_value(globals, context, for_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return false;
    } else if (t0 != TYPE_BOOL) {
        fprintf(globals->ferr, "Error: expected boolean as second parameter in for loop condition\n");
        return false;
    }
    ret = typecheck_statement(globals, context, for_stmt->increment);
    if (!ret) return false;
    return typecheck_statements(globals, context, for_stmt->stmts);
}

static bool typecheck_return(struct Globals *globals, struct TypeCheckerContext *context,
        struct Value *val)
{
    Type rettype = context->enclosing_func->rettype;
    if (val == NULL) {
        if (rettype == TYPE_UNDEFINED) {
            return true;
        } else {
            fprintf(globals->ferr, "Error: function '%s' should return value of type '%s' "
                    "but is returning nothing\n",
                    lookup_symbol(globals, context->enclosing_func->name),
                    lookup_symbol(globals, rettype));
            return false;
        }
    }
    Type val_type = typecheck_value(globals, context, val);
    if (val_type == TYPE_UNDEFINED) {
        return false;
    } else if (rettype == TYPE_UNDEFINED) {
        fprintf(globals->ferr, "Error: function '%s' should return nothing but is returning value of type '%s'\n",
                lookup_symbol(globals, context->enclosing_func->name),
                lookup_symbol(globals, val_type));
        return false;
    } else if (rettype != val_type) {
        fprintf(globals->ferr, "Error: function '%s' return type is '%s' but returning value of type '%s'\n",
                lookup_symbol(globals, context->enclosing_func->name),
                lookup_symbol(globals, rettype),
                lookup_symbol(globals, val_type));
        return false;
    }
    context->scope->rettype = val_type;
    return true;
}

static bool typecheck_break(struct Globals *globals, struct TypeCheckerContext *context)
{
    if (!is_in_loop(context->scope)) {
        fprintf(globals->ferr, "Error: break statement is not inside of a loop\n");
        return false;
    }
    return true;
}

static bool typecheck_continue(struct Globals *globals, struct TypeCheckerContext *context)
{
    if (!is_in_loop(context->scope)) {
        fprintf(globals->ferr, "Error: continue statement is not inside of a loop\n");
        return false;
    }
    return true;
}

static bool typecheck_increment(struct Globals *globals, struct TypeCheckerContext *context,
        struct Value *val)
{
    Type type = typecheck_value(globals, context, val);
    if (!(type == TYPE_INT || type == TYPE_BYTE || type == TYPE_FLOAT)) {
        fprintf(globals->ferr, "Error: cannot increment value of type '%s'\n", lookup_symbol(globals, type));
        return false;
    }
    return true;
}

static bool typecheck_funcall(struct Globals *globals, struct TypeCheckerContext *context,
        struct FunCall *funcall, struct FunDef *fundef)
{
    if (fundef == NULL) {
        fprintf(globals->ferr, "Error: no function named %s\n",
                lookup_symbol(globals, funcall->access->definition->name));
        return false;
    } 

    uint64_t fundef_arg_count  = fundef->args  == NULL ? 0 : fundef->args->length;
    uint64_t funcall_arg_count = funcall->args == NULL ? 0 : funcall->args->length;
    if (fundef_arg_count == 0 && funcall_arg_count == 0) {
        return true;
    } else if (fundef_arg_count != funcall_arg_count) {
        fprintf(globals->ferr, "Error: %s expects %" PRIu64 " arguments but got %" PRIu64 "\n",
                lookup_symbol(globals, fundef->name),
                fundef_arg_count, funcall_arg_count);
        return false;
    }

    // Validate arguments
    struct LinkedListNode *vals = funcall->args->head;
    struct LinkedListNode *defs_or_types = fundef->is_foreign
        ? fundef->types->head
        : fundef->args->head;
    for (uint64_t i = 0; i < funcall->args->length; i++) {
        struct Value *val = vals->value;
        Type fun_arg_def_type = TYPE_UNDEFINED;
        if (fundef->is_foreign) {
            Type *fun_arg_def_type_ptr = defs_or_types->value;
            fun_arg_def_type = *fun_arg_def_type_ptr;
        } else {
            struct Definition *fun_arg_def = defs_or_types->value;
            fun_arg_def_type = fun_arg_def->type;
        }
        Type val_type = typecheck_value(globals, context, val);
        if (val_type == TYPE_UNDEFINED) {
            return false;
        } else if (!(is_object(fun_arg_def_type) && val_type == TYPE_NULL)
                && (val_type != fun_arg_def_type)) {
            fprintf(globals->ferr, "Error: expected argument %" PRIu64 " to %s to be of type %s,"
                    " but got argument of type %s\n",
                    i + 1, lookup_symbol(globals, fundef->name),
                    lookup_symbol(globals, fun_arg_def_type),
                    lookup_symbol(globals, val_type));
            return false;
        }
        vals = vals->next;
        defs_or_types = defs_or_types->next;
    }
    return true;
}

// Typecheck for print functions
static bool typecheck_print(struct Globals *globals, struct TypeCheckerContext *context,
        Values *args)
{
    if (args == NULL || args->length == 0) {
        fprintf(globals->ferr, "Error: print function requires at least one argument\n");
        return false;
    }
    // Validate arguments
    linkedlist_foreach(lnode, args->head) {
        struct Value *val = lnode->value;
        Type val_type = typecheck_value(globals, context, val);
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
static Type typecheck_constructor(struct Globals *globals, struct TypeCheckerContext *context,
        struct Constructor *constructor)
{
    if (constructor->types != NULL) {
        fprintf(globals->ferr, "Error: class %s does not take type parameters\n",
                lookup_symbol(globals, constructor->funcall->access->definition->name));
        return TYPE_UNDEFINED;
    }
    struct FunCall *funcall = constructor->funcall;
    Symbol funname = funcall->access->definition->name;
    struct Class *cls = lookup_class(context->cls_table, funname);
    if (cls == NULL) {
        fprintf(globals->ferr, "Error: no class named %s\n", lookup_symbol(globals, funname));
        return TYPE_UNDEFINED;
    }

    uint64_t class_def_count       = cls->definitions  == NULL ? 0 : cls->definitions->length;
    uint64_t constructor_arg_count = funcall->args == NULL ? 0 : funcall->args->length;
    if (class_def_count == 0 && constructor_arg_count == 0) {
        return cls->name;
    } else if (class_def_count != constructor_arg_count) {
        fprintf(globals->ferr, "Error: %s expects %" PRIu64 " arguments but got %" PRIu64 "\n",
                lookup_symbol(globals, cls->name),
                class_def_count, constructor_arg_count);
        return TYPE_UNDEFINED;
    }

    // Validate arguments
    struct LinkedListNode *vals = funcall->args->head;
    struct LinkedListNode *defs = cls->definitions->head;
    for (uint64_t i = 0; i < funcall->args->length; i++) {
        struct Value *val = vals->value;
        struct Definition *fun_arg_def = defs->value;
        Type val_type = typecheck_value(globals, context, val);
        if (val_type == TYPE_UNDEFINED) {
            return false;
        // } else if (val_type != fun_arg_def->type) {
        } else if (!(is_object(fun_arg_def->type) && val_type == TYPE_NULL)
                && (val_type != fun_arg_def->type)) {
            fprintf(globals->ferr, "Error: expected argument %" PRIu64 " to %s to be of type %s, but got "
                    "argument of type %s\n",
                    i + 1, lookup_symbol(globals, cls->name),
                    lookup_symbol(globals, fun_arg_def->type),
                    lookup_symbol(globals, val_type));
            return TYPE_UNDEFINED;
        }
        vals = vals->next;
        defs = defs->next;
    }
    return cls->name;
}

static bool typecheck_definitions(struct Globals *globals, struct TypeCheckerContext *context,
        Definitions *defs)
{
    linkedlist_foreach(lnode, defs->head) {
        struct Definition *def = lnode->value;
        if (!typecheck_type(globals, context, def->type)) {
            return false;
        }
    }
    return true;
}

// TODO: make it check that all values are initialized
static bool typecheck_let(struct Globals *globals, struct TypeCheckerContext *context,
        Definitions *let)
{
    return scope_vars(globals, context, let) && typecheck_definitions(globals, context, let);
}

static bool typecheck_statement(struct Globals *globals, struct TypeCheckerContext *context,
        struct Statement *stmt)
{
    if (context->scope->rettype != TYPE_UNDEFINED) {
        fprintf(globals->ferr, "Error: statements below 'return' statement will not be executed\n");
        return false;
    }
    bool ret = false;
    switch (stmt->type) {
        case STMT_SET_LOCAL:
            ret = typecheck_set_local(globals, context, stmt->set_local);
            return ret;
        case STMT_SET_PROPERTY:
            ret = typecheck_set_property(globals, context, stmt->set_property);
            return ret;
        case STMT_SET_INDEX:
            ret = typecheck_set_index(globals, context, stmt->set_property);
            return ret;
        case STMT_LET:
            ret = typecheck_let(globals, context, stmt->let);
            return ret;
        case STMT_IF: {
            ret = typecheck_if(globals, context, stmt->if_stmt);
            return ret;
        }
        case STMT_WHILE:
            enter_scope(globals, &(context->scope), false, true);
            ret = typecheck_while(globals, context, stmt->while_stmt);
            exit_scope(&(context->scope));
            return ret;
        case STMT_FOR:
            enter_scope(globals, &(context->scope), false, true);
            ret = typecheck_for(globals, context, stmt->for_stmt);
            exit_scope(&(context->scope));
            return ret;
        case STMT_BUILTIN_FUNCALL: {
            Symbol name = stmt->funcall->access->definition->name;
            if (name == BUILTIN_PRINT) {
                ret = typecheck_print(globals, context, stmt->funcall->args);
            } else if (name == BUILTIN_PRINTLN) {
                // This is kind of a hack
                if (!linkedlist_is_empty(stmt->funcall->args)) {
                    linkedlist_append(stmt->funcall->args, new_string(globals, "\n"));
                }
                ret = typecheck_print(globals, context, stmt->funcall->args);
            } else {
                assert("Error: not implemented\n" && false);
            }
            return ret;
        }
        case STMT_FUNCALL: {
            Symbol funname = stmt->funcall->access->definition->name;
            struct FunDef *fundef = lookup_fun(context->fun_table, funname);
            ret = typecheck_funcall(globals, context, stmt->funcall, fundef);
            return ret;
        }
        case STMT_RETURN:
            return typecheck_return(globals, context, stmt->val);
        case STMT_BREAK:
            return typecheck_break(globals, context);
        case STMT_CONTINUE:
            return typecheck_continue(globals, context);
        case STMT_FOREACH:
            assert("Error: not implemented\n" && false);
            enter_scope(globals, &(context->scope), false, true);
            // ret = typecheck_foreach(globals, context, stmt->foreach_stmt);
            exit_scope(&(context->scope));
            return ret;
        case STMT_INC:
        case STMT_DEC:
            return typecheck_increment(globals, context, stmt->val);
        default:
            assert("Error, not implemented" && false);
            break;
    }
}

static bool typecheck_statements(struct Globals *globals, struct TypeCheckerContext *context,
        Statements *stmts)
{
    linkedlist_foreach(lnode, stmts->head) {
        struct Statement *stmt = lnode->value;
        if (!typecheck_statement(globals, context, stmt)) {
            return false;
        }
    }
    return true;
}

static bool typecheck_entrypoint(struct Globals *globals, struct TypeCheckerContext *context,
        struct FunDef *fundef)
{
    if (fundef->name == globals->entrypoint) {
        context->has_entrypoint = true;
        if (fundef->rettype != TYPE_UNDEFINED) {
            fprintf(globals->ferr, "Error: main function should not return any values\n");
            return false;
        } else if (!linkedlist_is_empty(fundef->args)) {
            fprintf(globals->ferr, "Error: main function should not take any arguments\n");
            return false;
        }
    }
    return true;
}

// Woah meta
static bool typecheck_type(struct Globals *globals, struct TypeCheckerContext *context, Type type)
{
    if (is_array(type)) {
        type = type_of_array(type);
    }
    if (is_object(type)) {
        struct Class *cls = lookup_class(context->cls_table, type);
        if (cls == NULL) {
            fprintf(globals->ferr, "Error: unknown type '%s'\n",
                    lookup_symbol(globals, type));
            return false;
        }
    }
    return true;
}

static bool typecheck_fundef(struct Globals *globals, struct TypeCheckerContext *context,
        struct FunDef *fundef)
{
    if (fundef->is_foreign) {
        // Can't really inspect body of foreign functions - assume they are correct
        return true;
    } else if (!typecheck_entrypoint(globals, context, fundef)) {
        return false;
    }
    struct Scope *scope = NULL;
    enter_scope(globals, &scope, true, false);
    context->enclosing_func = fundef;
    context->scope = scope;
    bool ret = scope_vars(globals, context, fundef->args) 
        && typecheck_definitions(globals, context, fundef->args)
        && typecheck_type(globals, context, fundef->rettype)
        && typecheck_statements(globals, context, fundef->stmts);
    if (!ret) {
        exit_scope(&scope);
        return false;
    }
    if (fundef->rettype == TYPE_UNDEFINED) {
        if (fundef->stmts->tail == NULL) {
            fprintf(globals->ferr, "Error: body of function '%s' is empty\n",
                    lookup_symbol(globals, fundef->name));
            return false;
        }
        struct Statement *stmt = fundef->stmts->tail->value;
        // Implicit return for functions that return nothing
        if (stmt->type != STMT_RETURN) {
            linkedlist_append(fundef->stmts, new_return(globals, NULL));
        }
        exit_scope(&scope);
        return ret;
    } else if (scope->rettype == TYPE_UNDEFINED) {
        fprintf(globals->ferr, "Error: function '%s' does not return a value on all paths\n",
                lookup_symbol(globals, fundef->name));
        return false;
    }
    exit_scope(&scope);
    return ret;
}

// Probably exists a less awful way to write this
static bool typecheck_class(struct Globals *globals, struct TypeCheckerContext *context,
        struct Class *cls)
{
    if (linkedlist_is_empty(cls->definitions)) {
        fprintf(globals->ferr, "Error: class '%s' declare with no fields\n",
                lookup_symbol(globals, cls->name));
        return false;
    }
    size_t i = 0;
    linkedlist_foreach(lnode, cls->definitions->head) {
        struct Definition *def = lnode->value;
        if (!typecheck_type(globals, context, def->type)) {
            return false;
        }
        size_t j = 0;
        linkedlist_foreach(inner_lnode, cls->definitions->head) {
            struct Definition *inner_def = inner_lnode->value;
            if (i != j && def->name == inner_def->name) {
                fprintf(globals->ferr, "Error: duplicate field name '%s' in class\n",
                        lookup_symbol(globals, def->name));
                return false;
            }
            j++;
        }
        j = 0;
        linkedlist_foreach(inner_lnode, cls->methods->head) {
            struct FunDef *method = ((struct TopLevelDecl *)inner_lnode->value)->fundef;
            if (def->name == method->name) {
                fprintf(globals->ferr, "Error: method and field cannot both have the same name '%s'\n",
                        lookup_symbol(globals, def->name));
                return false;
            }
            j++;
        }
        i++;
    }
    i = 0;
    linkedlist_foreach(lnode, cls->methods->head) {
        struct FunDef *method = ((struct TopLevelDecl *)lnode->value)->fundef;
        size_t j = 0;
        linkedlist_foreach(inner_lnode, cls->methods->head) {
            struct FunDef *inner_method = ((struct TopLevelDecl *)inner_lnode->value)->fundef;
            if (i != j && method->name == inner_method->name) {
                fprintf(globals->ferr, "Error: duplicate method name '%s' in class\n",
                        lookup_symbol(globals, method->name));
                return false;
            }
            j++;
        }
        i++;
    }
    return true;
}


static bool typecheck_tld(struct Globals *globals, struct TypeCheckerContext *context,
        struct TopLevelDecl *tld)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            return typecheck_class(globals, context, tld->cls);
        case TLD_TYPE_FUNDEF:
            return typecheck_fundef(globals, context, tld->fundef);
    }
    assert(false);
    return false;
}

static bool typecheck_tlds(struct Globals *globals, struct TypeCheckerContext *context,
        TopLevelDecls *tlds)
{
    linkedlist_foreach(lnode, tlds->head) {
        if (!typecheck_tld(globals, context, lnode->value)) {
            return false;
        }
    }
    return true;
}

static struct ClassTable *init_class_table(struct Globals *globals)
{
    struct Class *clst = DEL_MALLOC(globals->class_count * sizeof(*clst));
    struct ClassTable *class_table = DEL_MALLOC(sizeof(*class_table));
    class_table->size = globals->class_count;
    class_table->table = clst;
    return class_table;
}

static struct FunctionTable *init_function_table(struct Globals *globals)
{
    struct FunDef *ft = DEL_MALLOC(globals->function_count * sizeof(*ft));
    struct FunctionTable *function_table = DEL_MALLOC(sizeof(*function_table));
    function_table->size = globals->function_count;
    function_table->table = ft;
    return function_table;
}

static struct TypeCheckerContext *init_typechecker(struct Globals *globals)
{
    struct ClassTable *class_table = init_class_table(globals);
    struct FunctionTable *function_table = init_function_table(globals);
    // Init typechecker context
    struct TypeCheckerContext *context = DEL_MALLOC(sizeof(*context));
    context->has_entrypoint = false;
    context->enclosing_func = NULL;
    context->fun_table = function_table;
    context->cls_table = class_table;
    context->scope = NULL;
    // Init compiler context
    struct CompilerContext *cc = DEL_MALLOC(sizeof(*cc));
    cc->instructions = NULL;
    cc->funcall_table = NULL;
    cc->class_table = class_table;
    cc->fundef_table = function_table;
    globals->cc = cc;
    assert(globals->ast != NULL);
    return context;
}

bool typecheck(struct Globals *globals)
{
    struct TypeCheckerContext *context = init_typechecker(globals);
    if (!add_types(globals->ast, context->cls_table, context->fun_table)) {
        return false;
    } 
    bool is_success = typecheck_tlds(globals, context, globals->ast);
    if (is_success && !context->has_entrypoint) {
        fprintf(globals->ferr, "Error: program has no main function\n");
        return false;
    }
    // if (!is_success) fprintf(globals->ferr, "failed to typecheck\n");
    return is_success;
}

#undef find_open_loc

