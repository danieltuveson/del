#include "common.h"
#include "allocator.h"
#include "linkedlist.h"

static void add_symbol_helper(struct Globals *globals, char *sym, size_t size)
{
    char *symbol = allocator_malloc(globals->allocator, size);
    strcpy(symbol, sym);
    if (globals->symbol_table == NULL) {
        globals->symbol_table = linkedlist_new(globals->allocator);
    }
    linkedlist_append(globals->symbol_table, symbol);
}

#define add_symbol(sym) add_symbol_helper(globals, sym, sizeof(sym))

void init_symbol_table(struct Globals *globals)
{
    /* Add types to symbol table
     * Important Note: the order of these must be the same as the order in which they are
     * declared in the header if we want their symbol to correspond to the same number
     */
    add_symbol("**UNDEFINED**");
    add_symbol("null");
    add_symbol("bool");
    add_symbol("int");
    add_symbol("float");
    add_symbol("string");
    
    // Builtin functions
    add_symbol("print");
    add_symbol("println");
    add_symbol("read");
    add_symbol("concat");
}
#undef add_symbol

char *lookup_symbol(struct Globals *globals, uint64_t symbol)
{
    uint64_t cnt = 0;
    linkedlist_foreach(lnode, globals->symbol_table->head) {
        if (cnt == symbol) {
            return lnode->value;
        }
        cnt++;
    }
    return NULL;
}

bool is_builtin(uint64_t symbol)
{
    return symbol >= BUILTIN_PRINT && symbol <= BUILTIN_CONCAT;
}

