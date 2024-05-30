#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "linkedlist.h"

static void test_startmessage(char *name)
{
    printf("====== beginning tests for %s ======\n", name);
}

static void test_endmessage(char *name)
{
    printf("====== successfully finished testing %s ======\n", name);
}

// Helper function to test functionality of linkedlist_print
void string_printer(void *str)
{
    printf("%s", (char *) str);
}

void test_linkedlist(void)
{
    char testname[] = "linkedlist";
    test_startmessage(testname);

    struct LinkedList *ll = linkedlist_new();
    linkedlist_append(ll, "hi");
    linkedlist_append(ll, "thing");
    linkedlist_append(ll, "another");
    assert(ll->head->prev == NULL);
    assert(strcmp(ll->head->value, "hi") == 0);
    assert(strcmp(ll->head->next->value, "thing") == 0);
    assert(strcmp(ll->head->next->prev->value, "hi") == 0);
    assert(strcmp(ll->head->next->next->value, "another") == 0);
    assert(ll->head->next->next->next == NULL);
    assert(ll->head->next->next->next == NULL);
    linkedlist_print(ll, string_printer);
    linkedlist_reverse(&ll);
    assert(ll->head->prev == NULL);
    assert(strcmp(ll->head->value, "another") == 0);
    assert(strcmp(ll->head->next->value, "thing") == 0);
    assert(strcmp(ll->head->next->prev->value, "another") == 0);
    assert(strcmp(ll->head->next->next->value, "hi") == 0);
    assert(ll->head->next->next->next == NULL);
    assert(ll->head->next->next->next == NULL);
    linkedlist_print(ll, string_printer);

    test_endmessage(testname);
}

int main(void)
{
    test_linkedlist();
}