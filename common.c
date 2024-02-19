#include "common.h"

struct Ast ast = { NULL, 0, NULL };

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
    list->next = NULL;
    list->prev = NULL;
    return list;
}

struct List *append(struct List *list, void *value)
{
    list->prev = new_list(value);
    list->prev->next = list;
    return list->prev;
}

struct List *seek_end(struct List *list)
{
    for (; list->next != NULL; list = list->next);
    return list;
}
