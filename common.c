#include "common.h"
#include "allocator.h"
#include "linkedlist.h"

struct Globals globals = { {0}, NULL, NULL, NULL, NULL, 0, 0, 0, NULL };

uint64_t TYPE_UNDEFINED = 0;
uint64_t TYPE_NIL       = 1;
uint64_t TYPE_BOOL      = 2;
uint64_t TYPE_INT       = 3;
uint64_t TYPE_FLOAT     = 4;
uint64_t TYPE_STRING    = 5;

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

/* Important Note: the order of these must be the same as the order in which they are
 * declared above if we want their symbol to correspond to the same number
 */
void init_symbol_table(void)
{
    // Add types to symbol table
    add_symbol("**UNDEFINED**");
    add_symbol("null");
    add_symbol("bool");
    add_symbol("int");
    add_symbol("float");
    add_symbol("string");
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
