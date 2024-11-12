#include "common.h"
#include "allocator.h"
#include "readfile.h"

// Populates FileContext on success
bool readfile(struct Globals *globals, struct FileContext *file)
{
    FILE *fp = fopen(file->filename, "r");
    if (!fp) {
        printf("Error: could not open file '%s'\n", file->filename);
        return false;
    }
    char *generic_error = "Error: encountered error while reading file\n";
    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("%s", generic_error);
        return false;
    }
    file->length = ftell(fp);
    if (file->length == -1) {
        printf("%s", generic_error);
        return false;
    }
    rewind(fp);

    file->input = allocator_malloc(globals->allocator, file->length + 1);
    if ((long) fread(file->input, 1, file->length, fp) != file->length) {
        printf("%s", generic_error);
        return false;
    }
    fclose(fp);

    (file->input)[file->length] = '\0';
    return true;
}
