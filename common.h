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

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifndef EXPECT_ENABLED
#define EXPECT_ENABLED 1
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// https://clang.llvm.org/docs/LanguageExtensions.html
#if EXPECT_ENABLED && __has_builtin(__builtin_expect)
#define expected(x)   (__builtin_expect(((x) != 0), 1))
#define unexpected(x) (__builtin_expect(((x) != 0), 0))
#define DO_WE_HAVE_EXPECT() printf("__builtin_expect enabled\n")
#else
#define expected(x)   (x)
#define unexpected(x) (x)
#define DO_WE_HAVE_EXPECT() printf("__builtin_expect disabled\n")
#endif

#ifndef THREADED_CODE_ENABLED
#define THREADED_CODE_ENABLED 1
#endif

#if THREADED_CODE_ENABLED
#define DO_WE_HAVE_THREADING() printf("threading enabled\n")
#else
#define THREADED_CODE_ENABLED 0
#define DO_WE_HAVE_THREADING() printf("threading disabled\n")
#endif

#if DEBUG
#define DEBUG_GENERAL
#define DEBUG_LEXER 1
#define DEBUG_PARSER 1
#define DEBUG_TYPECHECKER 1
#define DEBUG_COMPILER 1
#define DEBUG_RUNTIME 1
#endif

// #define VM_INSTANCES_MAX        10000
// #define INSTRUCTIONS_MAX        100000
#define INSTRUCTIONS_MAX        UINT64_MAX
#define STACK_MAX               1000
// #define HEAP_MAX                1024
#define HEAP_INIT               128
#define HEAP_MAX                UINT64_MAX
#define ERROR_MESSAGE_MAX       250
#define GC_GROWTH_FACTOR 2

#define IN_BYTES(val) 8 * val
#define INSTRUCTIONS_MAX_BYTES        IN_BYTES(INSTRUCTIONS_MAX)
#define STACK_MAX_BYTES               IN_BYTES(STACK_MAX)
#define HEAP_MAX_BYTES                IN_BYTES(HEAP_MAX)

/*
 * A pointer to the heap consists of 3 parts:
 * - The first byte represents metadata about the heap value. TBD what goes here - I'm going to
 *   include the "mark" portion of mark and sweep gc as one of these bits. I also think I should
 *   include metadata for strings, like how many bytes are in the last uint64.
 * - The next 3 bytes stores the size of the data. For most objects this will be small, but
 *   arrays could possibly use up the full range.
 * - The last 32 bits store the location in the heap: that means our heap can store up to about 
 *   4 gigabytes of data before hitting this limit.
 */
#define COUNT_OFFSET    UINT64_C(32)
#define METADATA_OFFSET UINT64_C(56)
#define GC_MARK_OFFSET  UINT64_C(63)
#define METADATA_MASK   (UINT64_MAX - ((UINT64_C(1) << METADATA_OFFSET) - 1))
#define LOCATION_MASK   ((UINT64_C(1) << COUNT_OFFSET) - 1)
#define COUNT_MASK      (UINT64_MAX - (LOCATION_MASK + METADATA_MASK))
#define GC_MARK_MASK    (UINT64_C(1) << GC_MARK_OFFSET)


#define TODO() do { printf("Error: Not implemented\n"); assert(false); } while (false)

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
#define TYPE_ARRAY (UINT64_C(1) << 15)

/* Builtin functions */
#define BUILTIN_PRINT UINT64_C(6)
#define BUILTIN_PRINTLN UINT64_C(7)
#define BUILTIN_READ UINT64_C(8)
#define BUILTIN_CONCAT UINT64_C(9)
#define BUILTIN_ARRAY UINT64_C(10)

/* Builtin methods */
#define BUILTIN_CONSTRUCTOR UINT64_C(11)

/* Builtin variables */
#define BUILTIN_SELF UINT64_C(12)

#define BUILTIN_FIRST BUILTIN_PRINT
#define BUILTIN_LAST BUILTIN_SELF

static inline bool is_object(Type type)
{
    return type > BUILTIN_LAST || type == BUILTIN_ARRAY;
}

static inline bool is_array(Type type)
{
    return (TYPE_ARRAY & type) > 0;
}

static inline Type type_of_array(Type type)
{
    return ~TYPE_ARRAY & type;
}

/* List functions */
void init_symbol_table(struct Globals *globals);
// struct List *new_list(void *value);
// struct List *append(struct List *list, void *value);
// struct List *seek_end(struct List *list);
char *lookup_symbol(struct Globals *globals, uint64_t symbol);
bool is_builtin(uint64_t symbol);

#endif

