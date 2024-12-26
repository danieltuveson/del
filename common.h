#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "allocator.h"

// #define VM_INSTANCES_MAX        10000
// #define INSTRUCTIONS_MAX        100000
#define INSTRUCTIONS_MAX        UINT64_MAX
#define STACK_MAX               1000
// #define HEAP_MAX                1024
#define HEAP_MAX                UINT64_MAX
#define ERROR_MESSAGE_MAX       250

#define IN_BYTES(val) 8 * val
#define INSTRUCTIONS_MAX_BYTES        IN_BYTES(INSTRUCTIONS_MAX)
#define STACK_MAX_BYTES               IN_BYTES(STACK_MAX)
#define HEAP_MAX_BYTES                IN_BYTES(HEAP_MAX)

/* Symbol is used to represent any variable, function, or type name */
typedef uint64_t Symbol;

struct Globals {
    // Stores compile-time error message
    char error[ERROR_MESSAGE_MAX];
    // Stores main structs used throughout the program
    Allocator allocator;
    struct FileContext *file;
    struct Lexer *lexer;
    struct LinkedListNode *parser;
    // Used to store strings for each symbol
    struct LinkedList *symbol_table; 
    // Keep track of number of functions and classes parsed
    uint64_t class_count;
    uint64_t function_count;
    // Stores the symbol for "main"
    Symbol entrypoint;
    // Stores actual ast content (a list of top level definitions)
    struct LinkedList *ast;
    // Stores compiler context
    struct CompilerContext *cc;
};

/* Primitive that objects can have */
typedef uint64_t Type;
#define TYPE_UNDEFINED UINT64_C(0)
#define TYPE_NULL UINT64_C(1)
#define TYPE_BOOL UINT64_C(2)
#define TYPE_INT UINT64_C(3)
#define TYPE_FLOAT UINT64_C(4)
#define TYPE_STRING UINT64_C(5)

/* Array type modifies other types */
#define TYPE_ARRAY UINT64_C(1) << 63

/* Builtin functions */
#define BUILTIN_PRINT UINT64_C(6)
#define BUILTIN_PRINTLN UINT64_C(7)
#define BUILTIN_READ UINT64_C(8)
#define BUILTIN_CONCAT UINT64_C(9)
#define BUILTIN_ARRAY UINT64_C(10)

/* Builtin methods */
#define BUILTIN_CONSTRUCTOR UINT64_C(11)

#define BUILTIN_FIRST BUILTIN_PRINT
#define BUILTIN_LAST BUILTIN_CONSTRUCTOR

static inline bool is_object(Type val) { return val > BUILTIN_LAST; }

/* List functions */
void init_symbol_table(struct Globals *globals);
// struct List *new_list(void *value);
// struct List *append(struct List *list, void *value);
// struct List *seek_end(struct List *list);
char *lookup_symbol(struct Globals *globals, uint64_t symbol);
bool is_builtin(uint64_t symbol);

#endif

