#ifndef TXT_READER_H
#define TXT_READER_H

enum status openFile(FILE **file, const char *fileName, const char *mode);

enum status getFileSize(const char *fileName, size_t *size);

char **readLinesFromFile(const char *fileName, size_t *linesNum);


#endif
