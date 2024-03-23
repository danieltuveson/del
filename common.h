#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define INSTRUCTIONS_SIZE 100
#define STACK_SIZE 256
#define STACK_SIZE 256
#define HEAP_SIZE  1024

/* Symbol is used to represent any variable, function, or type name */
typedef uint64_t Symbol;

/* Doublely linked list used for various types in the AST.
 * Should always point to elements of the same type. */
struct List {
    void *value;
    uint64_t length; // number of "next" elements
    struct List *prev;
    struct List *next;
};

struct Ast {
    // Used to store strings for each symbol
    struct List *symbol_table; 
    // Keep track of number of functions and classes parsed
    uint64_t class_count;
    uint64_t function_count;
    // Stores the symbol for "main"
    Symbol entrypoint;
    // Stores actual ast content (a list of top level definitions)
    struct List *ast;
};

/* Types that objects can have */
typedef uint64_t Type;
extern uint64_t TYPE_UNDEFINED;
extern uint64_t TYPE_NIL;
extern uint64_t TYPE_INT;
extern uint64_t TYPE_FLOAT;
extern uint64_t TYPE_BOOL;
extern uint64_t TYPE_STRING;
static inline int is_object(Type val) { return val > TYPE_STRING; }

/* Global variable to hold ast of currently parsed ast
 * I would prefer to avoid globals, but I'm not sure how
 * to get Bison to return a value without one
 */
extern struct Ast ast;

/* List functions */
void init_symbol_table(void);
struct List *new_list(void *value);
struct List *append(struct List *list, void *value);
struct List *seek_end(struct List *list);
char *lookup_symbol(uint64_t symbol);


#endif

