#include "common.h"
#include "readfile.h"
#include "lexer.h"
#include "parser.h"
#include "error.h"

// Helper function to find the line where the error is happening
static void get_bad_line(char *text, char *underline, struct Token *token)
{
    // Seek line
    int line_start = 0;
    for (int line_count = 1; line_count < token->line_number; line_start++) {
        if (globals.file->input[line_start] == '\n') {
            line_count++;
        }
    }
    // Start / end indices
    int line_end = line_start;
    char *c = globals.file->input + line_start;
    for (char *c = globals.file->input + line_start; *c != '\n'; c++) {
        line_end++;
    }
    int length = line_end - line_start;
    // Copy problematic line to error buffer
    for (int i = 0; i < length && i < MAX_ERROR_MESSAGE_LENGTH - 1; i++) {
        text[i] = globals.file->input[i + line_start];
    }
    // Copy underline to buffer
    int i_arr = 0;
    for (int i = line_start; i < line_end && i_arr < MAX_ERROR_MESSAGE_LENGTH - 1; i++) {
        i_arr = i - line_start;
        if (i < token->start) {
            underline[i_arr] = ' ';
        } else if (i == token->start) {
            underline[i_arr] = '^';
        } else if (i < token->end) {
            underline[i_arr] = '~';
        } else {
            break;
        }
    }
}

// Making the intermediate arrays smaller to the truncation warnings go away
#define PRINTF_FUDGE_FACTOR_LENGTH MAX_ERROR_MESSAGE_LENGTH / 4

void error_parser(char *message, ...)
{
    char errormsg[PRINTF_FUDGE_FACTOR_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(errormsg, MAX_ERROR_MESSAGE_LENGTH, message, args);
    va_end(args);
    struct Token *token = globals.parser->head->value;
    char text[PRINTF_FUDGE_FACTOR_LENGTH] = {0};
    char underline[PRINTF_FUDGE_FACTOR_LENGTH] = {0};
    get_bad_line(text, underline, token);
    snprintf(
        globals.error,
        MAX_ERROR_MESSAGE_LENGTH,
        "Error in file '%s' at line %d, column %d\n"
        "%5d | %s\n        %s\n"
        "%s\n",
        globals.file->filename,
        token->line_number,
        token->column_number,
        token->line_number,
        text,
        underline,
        errormsg
    );
}

void error_print(void)
{
    printf("%s", globals.error);
}
