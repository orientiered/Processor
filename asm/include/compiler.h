#ifndef ASM_COMPILER_H
#define ASM_COMPILER_H

const size_t MAX_CODE_SIZE = 1000;
enum CMD_OPS {
    CMD_SNTXERR = -1,
    CMD_PUSH = 1,
    CMD_ADD,
    CMD_SUB,
    CMD_MUL,
    CMD_DIV,
    CMD_IN,
    CMD_OUT,
    CMD_HLT,
};

bool compile(const char *inName, const char *outName);

#endif
