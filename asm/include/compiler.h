#ifndef ASM_COMPILER_H
#define ASM_COMPILER_H

const size_t RESIZE_DELTA    = 128;
const size_t START_CODE_SIZE = 2048;
const size_t MAX_CMD_SIZE    = 128;
const char * const COMMENT_SYMBOLS = ";#";
const char * const LST_EXTENSION   = ".lst";
const int POISON_IP = -1;

typedef struct {
    int ip;
    char *label;
} label_t;

typedef struct {
    int ip;
    int index;
    size_t lineIdx;
} jmpLabel_t;

typedef struct {
    const char *inName;     ///< Name of input file
    const char *outName;    ///< Name of output file
    Vector_t labels;        ///< array with labels
    Vector_t fixup;         ///< array with unresolved labels

    char cmd[MAX_CMD_SIZE]; ///< String where words are scanned
    size_t reserved;
    int *code;              ///< Array with code //TODO: type for code, uint32_t or typedef
    int *ip;                ///< Pointer to current instruction in code

    char **codeLines;       ///< Lines of source code
    size_t lineCnt;         ///< Total number of lines
    size_t lineIdx;         ///< Current lines
} compilerData_t;


bool compile(const char *inName, const char *outName, bool makeListing);

#endif
