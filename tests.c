#include "common.h"
#include "allocator.h"
#include "linkedlist.h"
#include "lexer.h"
// #include "parser.h"

static void test_startmessage(char *name)
{
    printf("... %s tests ...\n", name);
}

static void test_endmessage(char *name)
{
    printf("... %s finished successfully ...\n", name);
}

// void test_example(void)
// {
//     char testname[] = "example";
//     test_startmessage(testname);
// 
//     test_endmessage(testname);
// }

#define l_init(input) lexer_init(&lexer, input, sizeof(input), false)

void test_parser(void)
{
    char testname[] = "parser";
    test_startmessage(testname);
    // char input[] = "(false || 4 / two == -2) && 5 < 6  + oijoji(x + new nested( "
    //                "functioncalls(wow, this_is, crazy * 100)) / 5)";
    // struct Lexer lexer;
    // l_init(input);
    // assert(tokenize(&lexer));

    // TODO: finish writing this test

    test_endmessage(testname);
}

void test_lexer(void)
{
    struct Token *first, *second, *third, *fourth, *fifth;
    char testname[] = "lexer";
    test_startmessage(testname);

    struct Lexer lexer;
    // Empty
    l_init("");
    assert(!tokenize(&lexer));

    // One character
    l_init("1");
    assert(tokenize(&lexer));
    print_lexer(&lexer);

    // Symbols and simple token
    l_init("hello, world");
    assert(tokenize(&lexer));
    assert(lexer.tokens->head != NULL);
    assert(lexer.tokens->head->next != NULL);
    assert(lexer.tokens->head->next->next != NULL);
    assert(lexer.tokens->head->next->next->next == NULL);
    first  = lexer.tokens->head->value;
    second = lexer.tokens->head->next->value;
    third  = lexer.tokens->head->next->next->value;
    assert(first->type == T_SYMBOL);
    assert(first->start == 0);
    assert(first->end == 5);
    assert(second->type == ST_COMMA);
    assert(second->start == 5);
    assert(second->end == 6);
    assert(third->type == T_SYMBOL);
    assert(third->start == 7);
    assert(third->end == 12);
    print_lexer(&lexer);

    // Numbers and strings
    l_init("1+2   /    \"blorp\"");
    assert(tokenize(&lexer));
    assert(lexer.tokens->head != NULL);
    assert(lexer.tokens->head->next != NULL);
    assert(lexer.tokens->head->next->next != NULL);
    assert(lexer.tokens->head->next->next->next != NULL);
    assert(lexer.tokens->head->next->next->next->next != NULL);
    assert(lexer.tokens->head->next->next->next->next->next == NULL);
    first  = lexer.tokens->head->value;
    second = lexer.tokens->head->next->value;
    third  = lexer.tokens->head->next->next->value;
    fourth = lexer.tokens->head->next->next->next->value;
    fifth  = lexer.tokens->head->next->next->next->next->value;
    assert(first->integer == 1);
    assert(second->type == ST_PLUS);
    assert(third->integer == 2);
    assert(fourth->type == ST_SLASH);
    assert(strcmp(fifth->string, "blorp") == 0);
    print_lexer(&lexer);

    // Bad input
    l_init("this character -> \a <- should *not* be in a valid program");
    assert(!tokenize(&lexer));

    test_endmessage(testname);
}

#undef l_init

// Helper function to test functionality of linkedlist_print
void string_printer(void *str)
{
    printf("%s", (char *) str);
}

void test_linkedlist(void)
{
    char testname[] = "linkedlist";
    test_startmessage(testname);

    // Create it, append stuff
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

    // Print result
    linkedlist_print(ll, string_printer);

    // Reverse
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
    test_lexer();
    printf("===== all tests completed successfully =====\n");
    allocator_freeall();
}