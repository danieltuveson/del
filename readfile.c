#include "common.h"
#include "allocator.h"

// Populates buffer with contents of file
// Returns 0 on failure, returns length of buffer (excluding null terminator) on success
long readfile(char **buff, char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: could not open file '%s'\n", filename);
        return 0;
    }
    char *generic_error = "Error: encountered error while reading file\n";
    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("%s", generic_error);
        return 0;
    }
    long length = ftell(fp);
    if (length == -1) {
        printf("%s", generic_error);
        return 0;
    }
    rewind(fp);

    *buff = allocator_malloc(length + 1);
    if ((long) fread(*buff, 1, length, fp) != length) {
        printf("%s", generic_error);
        return 0;
    }
    fclose(fp);

    (*buff)[length] = '\0';
    return length;
}