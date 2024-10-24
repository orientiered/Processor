#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include "cpuCommands.h"
#include "error_debug.h"
#include "utils.h"
#include "myVector.h"
#include "logger.h"
#include "txtReader.h"
#include "compiler.h"


static int cmdToReg(const char *cmd);
static enum CMD_OPS cmdToEnum(const char *cmd);
static bool checkSyntaxError(compilerData_t *comp, enum CMD_OPS error);

static int cmdToReg(const char *cmd) {
    assert(cmd);

    if (strlen(cmd) != 3)
        return CMD_SNTXERR;
    if (cmd[0] != 'r' || cmd[2] != 'x')
        return CMD_SNTXERR;
    if (cmd[1] > 'e' || cmd[1] < 'a')
        return CMD_SNTXERR;
    return cmd[1] - 'a' + 1; //registers count from 1
}

static enum CMD_OPS cmdToEnum(const char *cmd) {
    assert(cmd);

    for (size_t idx = 0; idx < ARRAY_SIZE(CPU_COMMANDS_ARRAY); idx++) {
        if (myStricmp(cmd, CPU_COMMANDS_ARRAY[idx].name) == 0)
            return CPU_COMMANDS_ARRAY[idx].op;
    }
    return CMD_SNTXERR;
}

static bool checkCmdBits(int *cmdPtr, int argType) {
    bool correctBits = true;
    bool IMMEDIATE = *cmdPtr & MASK_IMMEDIATE,
         REGISTER  = *cmdPtr & MASK_REGISTER,
         MEMORY    = *cmdPtr & MASK_MEMORY;

    if (argType == 1) {
        // M*!I*!R or !M*(I or !R)
        if      ( MEMORY && !IMMEDIATE && !REGISTER)
            correctBits = false;
        else if (!MEMORY && (IMMEDIATE || !REGISTER))
            correctBits = false;
    } else if (argType == 0) {
        if (!IMMEDIATE && !REGISTER)
            correctBits = false;
    }

    return correctBits;
}

/// argType = 0 -> arguments like push
/// argType = 1 -> arguments like pop
static bool scanArgs(compilerData_t *comp, char **line, int argType) {
    assert(comp);
    assert(line && *line);

    int scannedChars = 0;
    int *cmdPtr = comp->ip;
    comp->ip += CMD_LEN;
    if (sscanf(*line, " [ %d ] %n", comp->ip, &scannedChars) == 1 && scannedChars != 0) {
        *cmdPtr  |= MASK_MEMORY + MASK_IMMEDIATE;
        comp->ip += ARG_LEN;
    } else
    if (sscanf(*line, " [ %[^] \t\n+] ] %n", comp->cmd, &scannedChars) == 1 && scannedChars != 0) {
        *cmdPtr  |= MASK_MEMORY + MASK_REGISTER;
        *comp->ip = cmdToReg(comp->cmd);
        if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
            return false;
        comp->ip += REG_LEN;
    } else
    if (sscanf(*line, " [ %[^] +\t] + %d ] %n", comp->cmd, comp->ip + REG_LEN, &scannedChars) == 2 && scannedChars != 0) {
            *cmdPtr |= MASK_MEMORY + MASK_IMMEDIATE + MASK_REGISTER;
            *comp->ip = cmdToReg(comp->cmd);
            if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
                return false;
            comp->ip += REG_LEN + ARG_LEN;
    } else
    if (sscanf(*line, "%d%n", comp->ip, &scannedChars) == 1) {
        *cmdPtr |= MASK_IMMEDIATE;
        comp->ip += ARG_LEN;
    } else
    if (sscanf(*line, " %[^ \t\n+] + %d %n", comp->cmd, comp->ip + REG_LEN, &scannedChars) == 2) {
        *cmdPtr |= MASK_REGISTER + MASK_IMMEDIATE;
        *comp->ip = cmdToReg(comp->cmd);
        if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
            return false;
        comp->ip += REG_LEN + ARG_LEN;
    } else
    if (sscanf(*line, " %s %n", comp->cmd, &scannedChars) == 1) {
        *cmdPtr |= MASK_REGISTER;
        *comp->ip = cmdToReg(comp->cmd);
        if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
            return false;
        comp->ip += REG_LEN;
    }

    *line += scannedChars;

    if (!checkCmdBits(cmdPtr, argType)) {
        logPrint(L_ZERO, 1, "Syntax error in %s:%zu : ",
                comp->inName, comp->lineIdx+1);
        if (argType == 0)
            logPrint(L_ZERO, 1, "wrong push-like arguments:\n");
        else
            logPrint(L_ZERO, 1, "wrong pop-like  arguments:\n");

        logPrint(L_ZERO, 1, "\tMEMORY = %d, REGISTER = %d, IMMEDIATE = %d\n", bool(*cmdPtr & MASK_MEMORY),
                                                                              bool(*cmdPtr & MASK_REGISTER),
                                                                              bool(*cmdPtr & MASK_IMMEDIATE));
        return false;
    }

    logPrint(L_EXTRA, 0, "\tip =  0x%X\n", (size_t)(comp->ip - comp->code));
    logPrint(L_EXTRA, 0, "\tMEMORY = %d, REGISTER = %d, IMMEDIATE = %d\n", bool(*cmdPtr & MASK_MEMORY),
                                                                           bool(*cmdPtr & MASK_REGISTER),
                                                                           bool(*cmdPtr & MASK_IMMEDIATE));
    return true;
}

static bool checkIsLabel(char *line) {
    assert(line);
    //" %[a-zA_Z0-9]%*1[:] "
    char *symbol = line;
    for (;  isspace(*symbol) && *symbol != '\0'; symbol++)
        ;
    char *labelStart = symbol;
    for (; !isspace(*symbol) && *symbol != '\0'; symbol++)
        ;
    bool isLabel = (symbol > (labelStart + 1) && symbol[-1] == ':');
    if (!isLabel)
        return false;

    char temp = *symbol;
    *symbol = '\0';
    logPrint(L_EXTRA, 0, "\t'%s' is considered as label\n", labelStart);
    *symbol = temp;
    // if (sscanf(line, " %[a-zA-Z0-9_]%*1[:] ", cmd) == 1) {
    //     logPrint(L_EXTRA, 0, "\t'%s' is considered as label\n", cmd);
    //     return true;
    // }
    return true;
}

static label_t *findLabel(char *cmd, Vector_t* labels) {
    assert(cmd);
    assert(labels);

    if (labels->size == 0) return (label_t *) NULL;
    char *endIdx = vectorGetT(char*, labels, labels->size - 1);
    for (char *idx = (char *)labels->base; idx <= endIdx; idx += labels->elemSize) {
        if (strcmp(cmd, ((label_t *)idx)->label) == 0)
            return (label_t *)idx;
    }
    return (label_t *)NULL;
}

static bool processLabel(compilerData_t *comp) {
    assert(comp);

    char *line = comp->codeLines[comp->lineIdx];
    sscanf(line, " %[a-zA-Z0-9_]:", comp->cmd);
    logPrint(L_EXTRA, 0, "\tProcessing label '%s'\n", comp->cmd);
    label_t *label = findLabel(comp->cmd, &comp->labels);
    if (label == NULL) {
        label_t newLabel = {(size_t) (comp->ip - comp->code), strdup(comp->cmd)};
        vectorPush(&comp->labels, &newLabel);
        logPrint(L_EXTRA, 0, "\t\tAdding label name to array of labels: id = %d, ip = 0x%X\n",
                comp->labels.size - 1, (size_t) (comp->ip - comp->code));
    } else {
        if (label->ip != POISON_IP) {
            logPrint(L_ZERO, 1, "Syntax error in %s:%zu : redefined label '%s'\n",
                comp->inName, comp->lineIdx+1, comp->cmd);
            return false;
        } else {
            logPrint(L_EXTRA, 0, "\t\tAdding ip to label: ip = 0x%X\n", (size_t) (comp->ip - comp->code));
            label->ip = (size_t) (comp->ip - comp->code);
        }
    }
    return true;
}

static bool scanJmpLabel(compilerData_t *comp, char **line) {
    MY_ASSERT(comp && line && *line, abort());

    logPrint(L_EXTRA, 0, "\tJump '%s': ", comp->cmd);
    *comp->ip |= MASK_IMMEDIATE;
    comp->ip++;

    int scannedChars = 0;
    if (sscanf(*line, "%d%n", comp->ip, &scannedChars) == 1) {
        logPrint(L_EXTRA, 0, "ip = 0x%X\n", *comp->ip);
        comp->ip++;
        *line += scannedChars;
        return true;
    }

    //You can use label with or without ':' at the end
    if (sscanf(*line, " %[a-zA-Z0-9_]%n:%n ", comp->cmd, &scannedChars, &scannedChars) != 1) {
        logPrint(L_ZERO, 1, "Syntax error in %s:%zu : bad label in jump: '%s'\n",
            comp->inName, comp->lineIdx+1, comp->cmd);
        return false;
    }
    *line += scannedChars;
    logPrint(L_EXTRA, 0, "label = '%s'\n", comp->cmd);

    label_t *label = findLabel(comp->cmd, &comp->labels);
    if (label != NULL) {
        *comp->ip = label->ip;
        size_t labelIdx = size_t(label - (label_t*)comp->labels.base);
        logPrint(L_EXTRA, 0, "\t\tFound in labels, ip = 0x%X\n", label->ip);
        if (label->ip == POISON_IP) {
            jmpLabel_t jmpLabel = {size_t(comp->ip - comp->code), labelIdx, comp->lineIdx};
            vectorPush(&comp->fixup,  &jmpLabel);
            logPrint(L_EXTRA, 0, "\t\tAdding to fixup: idx = %zu\n", comp->fixup.size - 1);
        }
    } else {
        label_t newLabel = {-1, strdup(comp->cmd)};
        vectorPush(&comp->labels, &newLabel);
        logPrint(L_EXTRA, 0, "\t\tPush in labels, idx = %d, ip = 0x%X, line = %zu\n",
                             comp->labels.size - 1, (size_t)(comp->ip - comp->code), comp->lineIdx);
        jmpLabel_t jmpLabel = {size_t(comp->ip - comp->code), comp->labels.size - 1, comp->lineIdx};
        vectorPush(&comp->fixup,  &jmpLabel);
    }
    comp->ip++;
    return true;
}

/// @brief  Parse one line
/// Parsers for push, jmp, etc. move ip forward to next command, so in the end of cycle there's no comp->ip++
/// @return true on success, false otherwise
static bool parseCodeLine(compilerData_t *comp) {
    assert(comp);

    char *line = comp->codeLines[comp->lineIdx];
    int scannedChars = 0;
    logPrint(L_EXTRA, 0, "%s:%d: '%s'\n", comp->inName, comp->lineIdx, line);
    if (checkIsLabel(line))
        return processLabel(comp);

    while (sscanf(line, "%s%n", comp->cmd, &scannedChars) == 1) {
        line += scannedChars;

        *comp->ip = (int)cmdToEnum(comp->cmd);
        if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
            return false;
        logPrint(L_EXTRA, 0, "\tCmd: `%s` -> %d (ip = 0x%X)\n", comp->cmd, *comp->ip, (size_t)(comp->ip - comp->code));

        #define DEF_CMD_(cmdName, cmdIndex, argHandler, ...)    \
        case CMD_##cmdName: {                                   \
            if (!(argHandler))                                  \
                return false;                                   \
            break;                                              \
        }

        switch(*comp->ip) {
            #include "Commands.h"
            default:
            {
                logPrint(L_ZERO, 1, "It's possible that something went wrong (see %s:%d)\n", __FILE__, __LINE__);
                break;
            }
        }
        #undef DEF_CMD_
    }
    return true;
}

/// @brief  Strip line by any characters from separators
/// @return Pointer to new end of the line or NULL
static char *removeCommentFromLine(char *line, const char *separators) {
    assert(line && separators);

    for (char *symbol = line; *symbol != '\0'; symbol++) {
        for (const char *sep = separators; *sep != '\0'; sep++) {
            if (*symbol == *sep) {
                *symbol = '\0';
                return symbol;
            }
        }
    }
    return NULL;
}

static bool checkSyntaxError(compilerData_t *comp, enum CMD_OPS error) {
    assert(comp);

    if (error == CMD_SNTXERR) {
        logPrint(L_ZERO, 1, "\nSyntax error in %s:%zu : unknown command %s\n",
                comp->inName, comp->lineIdx+1, comp->cmd);
        return true;
    }
    return false;
}


static compilerData_t compilerDataCtor(const char *inName, const char *outName) {
    assert(inName && outName);

    compilerData_t comp = {0};
    comp.inName  = inName;
    comp.outName = outName;

    comp.labels = vectorCtor(0, sizeof(label_t));
    comp.fixup  = vectorCtor(0, sizeof(jmpLabel_t));

    memset(&comp.cmd, 0, MAX_CMD_SIZE);
    comp.code = (int *) calloc(START_CODE_SIZE, sizeof(int));
    comp.reserved = START_CODE_SIZE;
    comp.ip = comp.code;

    comp.codeLines = readLinesFromFile(inName, &comp.lineCnt);
    logPrint(L_EXTRA, 0, "------Program text---------\n");
    for (size_t idx = 0; idx < comp.lineCnt; idx++) {
        logPrint(L_EXTRA, 0, "%s\n", comp.codeLines[idx]);
        removeCommentFromLine(comp.codeLines[idx], COMMENT_SYMBOLS);
    }
    logPrint(L_EXTRA, 0, "------Assembling started---\n");
    comp.lineIdx = 0;

    return comp;
}

static bool compilerDataDtor(compilerData_t *comp) {
    assert(comp);

    free(comp->code);
    free(comp->codeLines[0]);
    free(comp->codeLines);

    for (size_t idx = 0; idx < comp->labels.size; idx++)
        free(((label_t *)vectorGet(&comp->labels, idx))->label);
    vectorDtor(&comp->labels);
    vectorDtor(&comp->fixup);
    logPrint(L_EXTRA, 0, "------Compilation end------\n");
    return true;
}

static bool writeCodeToFile(compilerData_t *comp) {
    assert(comp);

    FILE *outFile = fopen(comp->outName, "wb");
    if (!outFile) return false;

    size_t codeSize = size_t(comp->ip - comp->code);
    programHeader_t hdr = {*(const uint64_t *) CPU_SIGNATURE, CPU_CMD_VERSION, codeSize};
    fwrite(&hdr, 1, sizeof(hdr), outFile);
    //fprintf(outFile, "\n");
    // fprintf(outFile, "%s ", CPU_SIGNATURE);
    // fprintf(outFile, "%d %zu\n", CPU_CMD_VERSION, codeSize);
    // for (size_t idx = 0; idx < codeSize; idx++) {
    //     fprintf(outFile, "%x ", comp->code[idx]); //TODO: Format
    // }
    if (fwrite(comp->code, sizeof(int), codeSize, outFile) != codeSize) {
        logPrint(L_ZERO, 1, "Failed to write code in file\n");
        return false;
    }
    fclose(outFile);
    return true;
}

static bool fixupLabels(compilerData_t *comp) {
    assert(comp);

    logPrint(L_EXTRA, 0, "Resolving fixups: total %d\n", comp->fixup.size);
    for (size_t idx = 0; idx < comp->fixup.size; idx++) {
        jmpLabel_t *jmpLabel = vectorGetT(jmpLabel_t*, &comp->fixup, idx);
        size_t codeIdx = jmpLabel->ip;
        comp->code[codeIdx] = vectorGetT(label_t*, &comp->labels, jmpLabel->index)->ip;

        const char *labelString = vectorGetT(label_t*, &comp->labels, jmpLabel->index)->label;
        if (comp->code[codeIdx] == POISON_IP) {
            logPrint(L_ZERO, 1, "Undefined label at %s:%zu : '%s'\n",
                comp->inName, jmpLabel->lineIdx+1, labelString);
            return false;
        } else {
            logPrint(L_ZERO, 0, "\t(%d/%d) (ip=0x%.4X) Fixed label '%s'\n", idx + 1, comp->fixup.size, codeIdx, labelString);
        }
        percentageBar(idx + 1, comp->fixup.size, 40, 0);
    }
    return true;
}

static bool expandCodeArray(compilerData_t *comp) {
    MY_ASSERT(comp, abort());
    size_t curSize = size_t(comp->ip - comp->code);
    logPrint(L_DEBUG, 0, "Expanding code array:\n");
    logPrint(L_DEBUG, 0, "\tCurrent size = %zu, reserved = %zu\n", curSize, comp->reserved);

    comp->reserved *= 2;
    logPrint(L_DEBUG, 0, "\tNew reserved = %zu\n", comp->reserved);

    int *newCode = (int *) realloc(comp->code, comp->reserved * sizeof(int));
    if (!newCode) {
        logPrint(L_ZERO, 1, "Error occurred while expanding code array\n");
        return false;
    }
    comp->code = newCode;
    comp->ip   = comp->code + curSize;
    memset(comp->code + curSize, 0, comp->reserved - curSize);
    return true;
}

bool compile(const char *inName, const char *outName) {
    assert(inName && outName);

    logPrint(L_ZERO, 1, "--Reading program--\n");
    compilerData_t comp = compilerDataCtor(inName, outName);

    const size_t dotsCount = 40, skipCount = 16;
    logPrint(L_ZERO, 1, "--Parsing lines--\n");
    clock_t compilingTime = 0;
    clock_t startTime = clock();

    while(comp.lineIdx < comp.lineCnt) {
        if (!parseCodeLine(&comp)) {
            compilerDataDtor(&comp);
            return false;
        }

        if (size_t(comp.ip - comp.code) >= (comp.reserved - RESIZE_DELTA))
            expandCodeArray(&comp);

        comp.lineIdx++;
        compilingTime = clock() - startTime;
        if (comp.lineIdx % skipCount == 0)
            percentageBar(comp.lineIdx, comp.lineCnt, dotsCount, compilingTime);
    }

    logPrint(L_ZERO, 1, "\n--Fixing labels--\n");
    if (!fixupLabels(&comp)) {
        compilerDataDtor(&comp);
        return false;
    }

    logPrint(L_ZERO, 1, "--Writing assembled code--\n");
    bool compilationResult = writeCodeToFile(&comp);
    compilerDataDtor(&comp);
    return compilationResult;
}
