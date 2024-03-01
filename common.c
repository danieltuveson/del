#include "common.h"

struct Ast ast = { NULL, 0, 0, 0, NULL };

uint64_t TYPE_NIL    = 0;
uint64_t TYPE_BOOL   = 1;
uint64_t TYPE_INT    = 2;
uint64_t TYPE_FLOAT  = 3;
uint64_t TYPE_STRING = 4;

static void add_symbol_helper(char *sym, size_t size)
{
    char *symbol = malloc(size);
    strcpy(symbol, sym);
    if (ast.symbol_table == NULL) {
        ast.symbol_table = new_list(symbol);
    } else {
        ast.symbol_table = append(ast.symbol_table, symbol);
    }
}

#define add_symbol(sym) add_symbol_helper(sym, sizeof(sym))

/* Important Note: the order of these must be the same as the *reverse* order in which they are
 * declared above if we want their symbol to correspond to the same number
 */
void init_symbol_table(void)
{
    // Add types to symbol table
    add_symbol("string");
    add_symbol("float");
    add_symbol("int");
    add_symbol("bool");
    add_symbol("nil");
}
#undef add_symbol

char *lookup_symbol(uint64_t symbol)
{
    uint64_t cnt = 0;
    for (struct List *symbol_table = ast.symbol_table;
            symbol_table != NULL; symbol_table = symbol_table->next) {
        if (cnt == symbol) {
            return symbol_table->value;
        }
        cnt++;
    }
    return NULL;
}

/* List functions */
struct List *new_list(void *value)
{
    struct List *list = malloc(sizeof(struct List));
    list->value = value;
    list->length = 1;
    list->next = NULL;
    list->prev = NULL;
    return list;
}

struct List *append(struct List *list, void *value)
{
    list->prev = new_list(value);
    list->prev->length = list->length + 1;
    list->prev->next = list;
    return list->prev;
}

struct List *seek_end(struct List *list)
{
    for (; list->next != NULL; list = list->next);
    return list;
}

