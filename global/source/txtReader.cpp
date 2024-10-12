#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "error_debug.h"
#include "logger.h"
#include "txtReader.h"

enum status openFile(FILE **file, const char *fileName, const char *mode) {
    MY_ASSERT(file, abort());
    MY_ASSERT(fileName, abort());
    MY_ASSERT(mode, abort());

    *file = fopen(fileName, mode);
    if (!file) {
        logPrint(L_ZERO, 1, "Can't open file %s\n", fileName);
        return ERROR;
    }
    return SUCCESS;
}

enum status getFileSize(const char *fileName, size_t *size) {
    struct stat stBuf = {};
    if (!fileName || stat(fileName, &stBuf) == -1) {
        // logPrint(L_ZERO, 1, "Can't read file %s\n", fileName);
        logPrint(L_ZERO, 1, "Can't read file %s\n", fileName);
        return ERROR;
    }
    *size = (size_t)stBuf.st_size;
    return SUCCESS;
}


char **readLinesFromFile(const char *fileName) {
    MY_ASSERT(fileName, abort());
    size_t size = 0;
    getFileSize(fileName, &size);
    if (size == 0) {
        logPrint(L_ZERO, 1, "Something wrong with file '%s'\n", fileName);
        return NULL;
    }
    char *text = (char *) calloc(size, sizeof(char));
    FILE *file = 0;
    if (openFile(&file, fileName, "rb") != SUCCESS) {
        free(text);
        logPrint(L_ZERO, 1, "Can't open file '%s'\n", fileName);
        return NULL;
    }
    fread(text, sizeof(char), size, file);

    size_t linesCnt = 0;
    for (size_t idx = 0; idx < size; idx++) {
        if (text[idx] == '\n') linesCnt++;
    }
    if (text[size-1] != '\n') {
        linesCnt++;
        logPrint(L_ZERO, 1, "Suspicious file: no trailing \\n at the end (-_-)\n");
    }

    char **lines = (char **) calloc(linesCnt, sizeof(char*));

    size_t lineIdx = 1;
    lines[0] = text;
    bool prevIsEOL = false;
    for (size_t idx = 0; idx < size; idx++) {
        if (prevIsEOL) {
            prevIsEOL = false;
            lines[lineIdx++] = text + idx;
        }
        if (text[idx] == '\r') text[idx] = '\0';
        if (text[idx] == '\n') {
            text[idx] = '\0';
            prevIsEOL = true;
        }
    }

    return lines;
}
