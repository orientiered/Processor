#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "error_debug.h"
#include "utils.h"
#include "logger.h"
#include "cpuCommands.h"
#include "disasm.h"


static bool readHeader(FILE *programFile, size_t *codeSize) {
    MY_ASSERT(programFile && codeSize, abort());
    programHeader_t hdr = {};
    if (fread(&hdr, sizeof(hdr), 1, programFile) != 1) {
        logPrint(L_ZERO, 1, "Failed to read header\n");
        return false;
    }

    if (hdr.signature != *(const uint64_t *) CPU_SIGNATURE) {
        logPrint(L_ZERO, 1, "Wrong code signature: expected 0x%0.16LX, got 0x%0.16LX\n", CPU_SIGNATURE, hdr.signature);
        return false;
    }

    if (hdr.cmdVersion != CPU_CMD_VERSION) {
        logPrint(L_ZERO, 1, "Wrong command set version: expected %d, got %d\n", CPU_CMD_VERSION, hdr.cmdVersion);
        return false;
    }

    *codeSize = hdr.size;
    return true;
}

static bool readCode(const char *inputName, int **codeArray, size_t *codeSize) {
    FILE *in = fopen(inputName, "rb");
    if (!in) {
        logPrint(L_ZERO, 1, "Can't open file %s\n", inputName);
        return false;
    }

    if (!readHeader(in, codeSize))
        return false;

    *codeArray = (int *) calloc(*codeSize, sizeof(int));
    if (!codeArray) {
        logPrint(L_ZERO, 1, "Can't allocate memory for code\n");
        return false;
    }

    if (fread(*codeArray, sizeof(int), *codeSize, in) != *codeSize) {
        logPrint(L_ZERO, 1, "Failed to read code\n");
        return false;
    }

    fclose(in);
    return true;
}

static const char *enumToCmd(enum CMD_OPS cmdCode) {
    for (size_t idx = 0; idx < ARRAY_SIZE(CPU_COMMANDS_ARRAY); idx++) {
        if (CPU_COMMANDS_ARRAY[idx].op == cmdCode)
            return CPU_COMMANDS_ARRAY[idx].name;
    }
    return (const char *)NULL;
}

static bool writeCmd(FILE *out, int *code, int **ip) {
    int *cmdIp     = *ip;
    int  command   = **ip & MASK_CMD;
    bool REGISTER  = **ip & MASK_REGISTER;
    bool IMMEDIATE = **ip & MASK_IMMEDIATE;
    bool MEMORY    = **ip & MASK_MEMORY;

    const char *cmdName = enumToCmd(CMD_OPS(command));
    int printedChars = 0;
    if (cmdName == NULL) {
        logPrint(L_ZERO, 1, "Unknown command: %d\n", *cmdIp);
        return false;
    }
    printedChars += fprintf(out, "%-5s ", cmdName);

    *ip += CMD_LEN;
    if (MEMORY)
        printedChars += fprintf(out, "[");

    if (REGISTER) {
        printedChars += fprintf(out, "r%cx", 'a' + (**ip) - 1);
        *ip += REG_LEN;
    }
    if (IMMEDIATE) {
        if (REGISTER)
            printedChars += fprintf(out, " + ");
        printedChars += fprintf(out, "%d", (**ip) );
        *ip += ARG_LEN;
    }
    if (MEMORY)
        printedChars += fprintf(out, "]");

    for (size_t idx = printedChars; idx < MAX_LINE_LEN; idx++)
        fputc(' ', out);

    fprintf(out, "; |0x%08lX|> ", size_t(*ip - code));
    fprintf(out, "(%02X) ~%d%d%d|%02d~ ", *cmdIp, MEMORY, REGISTER, IMMEDIATE, command);
    cmdIp++;
    for (; cmdIp < *ip; cmdIp++)
        fprintf(out, "%08lX ", *cmdIp);
    fprintf(out, "\n");

    return true;
}

bool disasm(const char *inputName, const char* outputName) {
    MY_ASSERT(inputName && outputName, abort());
    int *code = NULL;
    size_t codeSize = 0;
    if (!readCode(inputName, &code, &codeSize))
        return false;

    FILE *out = fopen(outputName, "wb");
    fprintf(out, "; This code was generated using disassembler\n");

    int *ip = code;
    while (ip < code + codeSize) {
        if (!writeCmd(out, code, &ip)) {
            free(code);
            fclose(out);
            return false;
        }
    }

    free(code);
    fclose(out);
    return true;
}

