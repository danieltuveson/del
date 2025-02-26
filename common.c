#include "common.h"
#include "allocator.h"
#include "linkedlist.h"


static void add_sym_helper(struct Globals *globals, char *sym, size_t size)
{
    char *symbol = allocator_malloc(globals->allocator, size);
    strcpy(symbol, sym);
    if (globals->symbol_table == NULL) {
        globals->symbol_table = linkedlist_new(globals->allocator);
    }
    linkedlist_append(globals->symbol_table, symbol);
}

#define add_sym(sym) add_sym_helper(globals, sym, sizeof(sym))

void init_symbol_table(struct Globals *globals)
{
    /* Add types to symbol table
     * Important Note: the order of these must be the same as the order in which they are
     * declared in the header if we want their symbol to correspond to the same number
     */
    add_sym("**UNDEFINED**");
    add_sym("null");
    add_sym("bool");
    add_sym("int");
    add_sym("float");
    add_sym("byte");
    add_sym("string");

    // Builtin functions
    add_sym("print");
    add_sym("println");
    add_sym("read");
    add_sym("concat");
    add_sym("Array");

    // Builtin methods 
    add_sym("constructor");

    // Builtin properties
    add_sym("length");

    // Reserved variable names
    add_sym("self");
}
#undef add_symbol

Symbol add_symbol(struct Globals *globals, char *str, int str_len)
{
    /* Adds symbols to symbol table */
    struct LinkedListNode *symbol_table = globals->symbol_table->head;
    uint64_t cnt = 0;
    char *symbol;
    while (1) {
        if (strcmp(symbol_table->value, str) == 0) {
            goto addsymbol;
        }
        cnt++;
        if (symbol_table->next == NULL) {
            break;
        } else {
            symbol_table = symbol_table->next;
        }
    }
    symbol = allocator_malloc(globals->allocator, (str_len + 1) * sizeof(char));
    strcpy(symbol, str);
    // symbol_table->next = new_list(symbol);
    linkedlist_append(globals->symbol_table, symbol);
addsymbol:
    if (strcmp(str, "main") == 0) {
        globals->entrypoint = cnt;
    }
    return cnt;
}


char *lookup_symbol(struct Globals *globals, uint64_t symbol)
{
    if (symbol >= TYPE_ARRAY) {
        return "Array";
    }
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
    return symbol >= BUILTIN_FIRST && symbol <= BUILTIN_LAST;
}

