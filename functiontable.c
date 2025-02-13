#include "allocator.h"
#include "linkedlist.h"
#include "functiontable.h"
#include "printers.h"

struct FunctionCallTable *new_ft(struct Globals *globals, uint64_t function)
{
    struct FunctionCallTable *ft = allocator_malloc(globals->allocator, sizeof(struct FunctionCallTable));
    ft->node = allocator_malloc(globals->allocator, sizeof(struct FunctionCallTableNode));
    ft->node->function = function;
    ft->node->location = 0;
    ft->node->callsites = linkedlist_new(globals->allocator);
    ft->left = NULL;
    ft->right = NULL;
    return ft;
}

static inline struct FunctionCallTableNode *add_function_internal(struct Globals *globals, struct FunctionCallTable *ft, 
       Symbol function, uint64_t loc, int noloc)
{
    int from_left = 0;
    struct FunctionCallTable *prev = NULL;
    for (;;) {
        if (ft == NULL) {
            ft = new_ft(globals, function);
            if (!noloc) ft->node->location = loc;
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
            /* function exists, update location if applicable */
            if (!noloc) ft->node->location = loc;
            break;
        }
    }
    return ft->node;
}

// Adds function to table, *does not* set the location of the function call
#define add_function_noloc(globals, ft, function) \
    add_function_internal(globals, ft, function, 0, 1)

// Adds function to table, *does* set the location of the function call
struct FunctionCallTableNode *add_ft_node(struct Globals *globals, struct FunctionCallTable *ft, Symbol function, uint64_t loc)
{
    return add_function_internal(globals, ft, function, loc, 0);
}

void add_callsite(struct Globals *globals, struct FunctionCallTable *ft, Symbol function, uint64_t callsite)
{
    struct FunctionCallTableNode *node = add_function_noloc(globals, ft, function);
    uint64_t *hcallsite = allocator_malloc(globals->allocator, sizeof(uint64_t));
    *hcallsite = callsite;
    linkedlist_append(node->callsites, hcallsite);
}

#undef add_function_noloc
