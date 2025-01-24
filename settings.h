#ifndef SETTINGS_H
#define SETTINGS_H

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

#if DEBUG_ALL
#define DEBUG_GENERAL 1
#define DEBUG_TEXT 1
#define DEBUG_LEXER 1
#define DEBUG_PARSER 1
#define DEBUG_TYPECHECKER 1
#define DEBUG_COMPILER 1
#define DEBUG_RUNTIME 1
#define DEBUG
#endif

#ifndef DEBUG_GENERAL
#define DEBUG_GENERAL 0
#endif
#ifndef DEBUG_TEXT
#define DEBUG_TEXT 0
#endif
#ifndef DEBUG_LEXER
#define DEBUG_LEXER 0
#endif
#ifndef DEBUG_PARSER
#define DEBUG_PARSER 0
#endif
#ifndef DEBUG_TYPECHECKER
#define DEBUG_TYPECHECKER 0
#endif
#ifndef DEBUG_COMPILER
#define DEBUG_COMPILER 0
#endif
#ifndef DEBUG_RUNTIME
#define DEBUG_RUNTIME 0
#endif

// #define VM_INSTANCES_MAX        10000
// #define INSTRUCTIONS_MAX        100000
// WARNING: Currently changing INSTRUCTIONS_MAX to a lower number will result in it failing at 
// runtime. The safest way to restrict the maximum upper bound of compiler memory, restrict the
// maximum allowed size of a program input file.
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

#endif
