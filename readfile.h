#ifndef READFILE_H
#define READFILE_H

struct FileContext {
    // File name of file being parsed
    char *filename;
    // Length of file contents (excludes null terminator)
    long length;
    // Stores contents of file being parsed
    char *input;
};

bool readfile(struct FileContext *file);

#endif
