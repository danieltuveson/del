#include "functiontable.h"

struct FunctionTable *new_ft(uint64_t function)
{
    struct FunctionTable *ft = malloc(sizeof(struct FunctionTable));
    ft->node = malloc(sizeof(struct FunctionTableNode));
    ft->node->function = function;
    ft->node->callsites = NULL;
    ft->left = NULL;
    ft->right = NULL;
    return ft;
}

struct FunctionTableNode *add_function(struct FunctionTable *ft, Symbol function)
{
    int from_left = 0;
    struct FunctionTable *prev = NULL;
    for (;;) {
        if (ft == NULL) {
            ft = new_ft(function);
            if (from_left) {
                prev->left = ft;
            } else {
                prev->right = ft;
            }
            break;
        } else if (function < ft->node->function) {
            from_left = 1;
            prev = ft;
            ft = ft->left;
        } else if (function > ft->node->function) {
            from_left = 0;
            prev = ft;
            ft = ft->right;
        } else /* if (function == ft->node->function) */ {
            /* function exists, do nothing */
            break;
        }
    }
    return ft->node;
}

void add_callsite(struct FunctionTable *ft, Symbol function, uint64_t callsite)
{
    struct FunctionTableNode *node = add_function(ft, function);
    int *hcallsite = malloc(sizeof(uint64_t));
    *hcallsite = callsite;
    if (node->callsites == NULL) {
        node->callsites = new_list(hcallsite);
    } else {
        append(node->callsites, hcallsite);
    }
}

