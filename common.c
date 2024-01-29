#include "common.h"

struct Ast ast = { NULL, NULL };

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
