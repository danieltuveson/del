#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#define INSTRUCTIONS_SIZE 10000
#define STACK_SIZE 256
#define HEAP_SIZE  1024
#define MAX_ERROR_MESSAGE_LENGTH 250

/* Symbol is used to represent any variable, function, or type name */
typedef uint64_t Symbol;

struct Globals {
    // Stores compile-time error message
    char error[MAX_ERROR_MESSAGE_LENGTH];
    // Stores main structs used throughout the program
    struct FileContext *file;
    struct Lexer *lexer;
    struct Parser *parser;
    // Used to store strings for each symbol
    struct LinkedList *symbol_table; 
    // Keep track of number of functions and classes parsed
    uint64_t class_count;
    uint64_t function_count;
    // Stores the symbol for "main"
    Symbol entrypoint;
    // Stores actual ast content (a list of top level definitions)
    struct LinkedList *ast;
};

/* Types that objects can have */
typedef uint64_t Type;
extern uint64_t TYPE_UNDEFINED;
extern uint64_t TYPE_NIL;
extern uint64_t TYPE_INT;
extern uint64_t TYPE_FLOAT;
extern uint64_t TYPE_BOOL;
extern uint64_t TYPE_STRING;
static inline bool is_object(Type val) { return val > TYPE_STRING; }

// Global variable to hold ast of currently parsed ast
extern struct Globals globals;

/* List functions */
void init_symbol_table(void);
// struct List *new_list(void *value);
// struct List *append(struct List *list, void *value);
// struct List *seek_end(struct List *list);
char *lookup_symbol(uint64_t symbol);

#endif

