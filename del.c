#include "common.h"
#include "allocator.h"
#include "linkedlist.h"
#include "readfile.h"
#include "lexer.h"
#include "error.h"
#include "ffi.h"
#include "ast.h"
#include "parser.h"
#include "typecheck.h"
#include "compiler.h"
#include "printers.h"
#include "vector.h"
#include "del.h"

static bool parse_and_compile(struct Globals *globals, struct Program **program)
{
#if DEBUG_LEXER
    printf("........ TOKENIZING INPUT ........\n");
#endif
    struct Lexer lexer;
    lexer_init(globals, &lexer, false);
    globals->lexer = &lexer;
    if (!tokenize(globals)) {
        fprintf(globals->ferr, "Error at line %d column %d: %s\n",
                globals->lexer->error.line_number,
                globals->lexer->error.column_number,
                globals->lexer->error.message);
        return false;
    }
#if DEBUG_LEXER
    print_lexer(globals, globals->lexer);
    print_memory_usage(globals->allocator);
#endif

#if DEBUG_LEXER
    printf("........ PRINTING ALL SYMBOLS ........\n");
    linkedlist_foreach(lnode, globals->symbol_table->head) {
        printf("symbol: '%s'\n", (char *) lnode->value);
    }
#endif

#if DEBUG_FFI
    printf("........ REGISTERING FOREIGN FUNCTIONS ........\n");
#endif
    if (!ffi_register_functions(globals)) {
        fprintf(globals->ferr, "Error registering foreign function\n");
        return false;
    }

#if DEBUG_PARSER
    printf("........ PARSING AST FROM TOKENS ........\n");
#endif
    // struct Parser parser = { globals->lexer.tokens->head, &lexer };
    // globals->parser = &parser;
    globals->parser = globals->lexer->tokens->head;
    if (!parse_tlds(globals)) {
        error_print(globals);
        return false;
    }
#if DEBUG_PARSER
    print_tlds(globals, globals->ast);
    printf("\n");
    print_memory_usage(globals->allocator);

    printf("````````````````` CODE `````````````````\n");
    print_tlds(globals, globals->ast);
    printf("\n");

#endif
#if DEBUG_TYPECHECKER
    printf("`````````````` TYPECHECK ```````````````\n");
#endif
    if (typecheck(globals)) {
#if DEBUG_TYPECHECKER
        printf("program has typechecked\n");
#endif
    } else {
#if DEBUG_TYPECHECKER
        printf("program failed to typecheck\n");
#endif
        return false;
    }
#if DEBUG_COMPILER
    printf("`````````````` COMPILE ```````````````\n");
#endif
    compile(globals, globals->ast);
    *program = malloc(sizeof(**program));
    (*program)->instructions = globals->cc->instructions;
    (*program)->string_count = globals->cc->string_count;
    (*program)->string_pool = globals->cc->string_pool;
#if DEBUG_COMPILER
    printf("\n");
    printf("````````````` INSTRUCTIONS `````````````\n");
    printf("function table:\n");
    print_ft(globals, globals->cc->funcall_table);
    printf("\n");
    print_instructions(globals->cc);
    printf("\n");
#endif
    return true;
}

#define INIT_GLOBALS()\
{ {0}, stderr, 0, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL }

static bool parse_and_compile_file(struct Globals *globals, struct Program **program,
        char *filename)
{

#if DEBUG_TEXT
    printf("........ READING FILE : %s ........\n", filename);
#endif
    struct FileContext file = { filename, 0, NULL };
    if (!readfile(globals, &file)) {
        fprintf(globals->ferr, "Error: could not read contents of empty file\n");
        return false;
    }

    globals->file = &file;
#if DEBUG_TEXT
    printf("%s\n", globals->file->input);
    print_memory_usage(globals->allocator);
#endif
    return parse_and_compile(globals, program);
}

static bool parse_and_compile_text(struct Globals *globals, struct Program **program,
        char *program_text)
{
    struct FileContext file = { NULL, strlen(program_text), program_text };
    globals->file = &file;
    return parse_and_compile(globals, program);
}

void del_compiler_init(DelCompiler *compiler, FILE *ferr)
{
    struct Globals *globals = malloc(sizeof(*globals));
    memset(globals, 0, sizeof(*globals));
    globals->allocator = allocator_new();
    globals->ferr = ferr;
    globals->ast = linkedlist_new(globals->allocator);
    init_symbol_table(globals);
    globals->foreign_function_table = linkedlist_new(globals->allocator);
    *compiler = (DelCompiler) globals;
}

void del_compiler_free(DelCompiler compiler)
{
    struct Globals *globals = (struct Globals *) compiler;
    allocator_freeall(globals->allocator);
    free(globals);
}

void del_register_function_helper(DelCompiler compiler, void *context, bool is_yielding,
        DelForeignFunctionCall function, char *ff_name, int arg_count, ...)
{
    struct Globals *globals = (struct Globals *) compiler;
    struct LinkedList *types = linkedlist_new(globals->allocator);
    va_list arg_list;
    va_start(arg_list, arg_count);
    enum DelForeignType rettype = va_arg(arg_list, enum DelForeignType);
    for (int i = 0; i < arg_count; i++) {
        enum DelForeignType dtype = va_arg(arg_list, enum DelForeignType);
        Type *type_ptr = allocator_malloc(globals->allocator, sizeof(*type_ptr));
        *type_ptr = convert_ffi_type(dtype);
        linkedlist_append(types, type_ptr);
    }
    va_end(arg_list);
    ffi_register_function(globals, context, is_yielding, function, ff_name, rettype, types);
}

DelProgram del_compile_text(DelCompiler compiler, char *program_text)
{
    struct Program *program = NULL;
    struct Globals *globals = (struct Globals *) compiler;
    if (parse_and_compile_text(globals, &program, program_text)) {
        DelProgram del_program = (DelProgram) program;
        return del_program;
    }
    return 0;
}

DelProgram del_compile_file(DelCompiler compiler, char *filename)
{
    struct Program *program = NULL;
    struct Globals *globals = (struct Globals *) compiler;
    if (parse_and_compile_file(globals, &program, filename)) {
        DelProgram del_program = (DelProgram) program;
        return del_program;
    }
    return 0;
}

void del_program_free(DelProgram del_program)
{
    struct Program *program = (struct Program *) del_program;
    vector_free(program->instructions);
    for (size_t i = 0; i < program->string_count; i++) {
        free(program->string_pool[i]);
    }
    if (program->string_pool != NULL) free(program->string_pool);
    free(program);
}

void del_vm_init(DelVM *del_vm, FILE *fout, FILE *ferr, DelProgram del_program)
{
    struct VirtualMachine *vm = malloc(sizeof(*vm));
    memset(vm, 0, sizeof(*vm));
    struct Program *program = (struct Program *) del_program;
    vm_init(vm, fout, ferr, program->instructions->values, program->string_pool);
    *del_vm = (DelVM) vm;
}

void del_vm_execute(DelVM del_vm)
{
    struct VirtualMachine *vm = (struct VirtualMachine *) del_vm;
    vm_execute(vm);
}

enum DelVirtualMachineStatus del_vm_status(DelVM del_vm)
{
    struct VirtualMachine *vm = (struct VirtualMachine *) del_vm;
    return vm->status;
}

void del_vm_free(DelVM del_vm)
{
    struct VirtualMachine *vm = (struct VirtualMachine *) del_vm;
    vm_free(vm);
    free(vm);
}

