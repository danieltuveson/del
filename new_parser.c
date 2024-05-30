#include "common.h"
#include "linkedlist.h"
#include "lexer.h"
#include "readfile.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage: ./del hello_world.del\n");
        return 1;
    }
    printf("........ READING FILE : %s ........\n", argv[1]);
    char *input = NULL;
    int input_length = readfile(&input, argv[1]);
    if (input_length == 0) {
        return 1;
    }
    printf("%s\n", input);
    printf("........ TOKENIZING INPUT ........\n");
    struct CompilerError error = { NULL, 1, 1 };
    struct Lexer lexer = {
        &error,
        input_length,
        input,
        0,
        NULL
    };
    tokenize(&lexer);
    if (lexer.error->message) {
        print_error(lexer.error);
        return 1;
    }
    print_lexer(&lexer);

    return 0;
}
