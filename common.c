#include "common.h"
#include "allocator.h"
#include "linkedlist.h"

struct Globals globals = { {0}, NULL, NULL, NULL, NULL, 0, 0, 0, NULL };

static void add_symbol_helper(char *sym, size_t size)
{
    char *symbol = allocator_malloc(size);
    strcpy(symbol, sym);
    if (globals.symbol_table == NULL) {
        globals.symbol_table = linkedlist_new();
    }
    linkedlist_append(globals.symbol_table, symbol);
}

#define add_symbol(sym) add_symbol_helper(sym, sizeof(sym))

void init_symbol_table(void)
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

char *lookup_symbol(uint64_t symbol)
{
    uint64_t cnt = 0;
    // printf("something bad happens here?\n");
    linkedlist_foreach(lnode, globals.symbol_table->head) {
        // printf("looking...\n");
        if (cnt == symbol) {
            // printf("nope, returned without issue\n");
            return lnode->value;
        }
        cnt++;
    }
    // printf("returns null. This should not happen!!!\n");
    return NULL;
}

bool is_builtin(uint64_t symbol)
{
    return symbol >= BUILTIN_PRINT && symbol <= BUILTIN_CONCAT;
}

