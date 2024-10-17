#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

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
    if (strlen(cmd) != 3)
        return CMD_SNTXERR;
    if (cmd[0] != 'r' || cmd[2] != 'x')
        return CMD_SNTXERR;
    if (cmd[1] > 'e' || cmd[1] < 'a')
        return CMD_SNTXERR;
    return cmd[1] - 'a' + 1; //registers count from 1
}

static enum CMD_OPS cmdToEnum(const char *cmd) {
    for (size_t idx = 0; idx < ARRAY_SIZE(CPU_COMMANDS_ARRAY); idx++) {
        if (strcmp(cmd, CPU_COMMANDS_ARRAY[idx].name) == 0)
            return CPU_COMMANDS_ARRAY[idx].op;
    }
    return CMD_SNTXERR;
}

static bool scanPushArgs(compilerData_t *comp, char **line) {
    int scannedChars = 0;
    int *pushIp = comp->ip;
    comp->ip++;
    if (sscanf(*line, "%d%n", comp->ip, &scannedChars) == 1) {
        *line += scannedChars;
        *pushIp |= MASK_IMMEDIATE;
    } else {
        sscanf(*line, "%s%n", comp->cmd, &scannedChars);
        *line += scannedChars;
        *pushIp |= MASK_REGISTER;
        *comp->ip = cmdToReg(comp->cmd);
        if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
            return false;
        if (sscanf(*line, "%d%n", comp->ip + 1, &scannedChars) == 1) {
            comp->ip++;
            *line += scannedChars;
            *pushIp |= MASK_IMMEDIATE;
        }
    }
    logPrint(L_EXTRA, 0, "\tScanned %d push arguments\n", (int)(comp->ip - pushIp));
    comp->ip++;
    return true;
}

static bool checkIsLabel(char *line) {
    char *symbol = line;
    for (;  isspace(*symbol) && *symbol != '\0'; symbol++) ;
    char *labelStart = symbol;
    for (; !isspace(*symbol) && *symbol != '\0'; symbol++) ;
    bool isLabel = (symbol > (labelStart + 1) && symbol[-1] == ':');
    if (!isLabel)
        return false;

    char temp = *symbol;
    *symbol = '\0';
    logPrint(L_EXTRA, 0, "\t'%s' is considered as label\n", labelStart);
    *symbol = temp;
    return true;
}

static label_t *findLabel(char *cmd, Vector_t* labels) {
    if (labels->size == 0) return (label_t *) NULL;
    char *endIdx = (char *)vectorGet(labels, labels->size - 1);
    for (char *idx = (char *)labels->base; idx <= endIdx; idx += labels->elemSize) {
        if (strcmp(cmd, ((label_t *)idx)->label) == 0)
            return (label_t *)idx;
    }
    return (label_t *)NULL;
}

static bool processLabel(compilerData_t *comp) {
    char *line = comp->codeLines[comp->lineIdx];
    sscanf(line, "%s", comp->cmd);
    logPrint(L_EXTRA, 0, "\tProcessing label '%s'\n", comp->cmd);
    label_t *label = findLabel(comp->cmd, &comp->labels);
    if (label == NULL) {
        label_t newLabel = {(int) (comp->ip - comp->code), strdup(comp->cmd)};
        vectorPush(&comp->labels, &newLabel);
    } else {
        if (label->ip != POISON_IP) {
            logPrint(L_ZERO, 1, "Syntax error in %s:%zu : redefined label '%s'\n",
                comp->inName, comp->lineIdx+1, comp->cmd);
            return false;
        } else {
            label->ip = (int) (comp->ip - comp->code);
        }
    }
    return true;
}

static bool processJmpLabel(compilerData_t *comp, char **line) {
    MY_ASSERT(comp && line && *line, abort());

    logPrint(L_EXTRA, 0, "\tJump '%s': ", comp->cmd);
    comp->ip++;

    int scannedChars = 0;
    if (sscanf(*line, "%d%n", comp->ip, &scannedChars) == 1) {
        logPrint(L_EXTRA, 0, "ip = %d\n", *comp->ip);
        comp->ip++;
        *line += scannedChars;
        return true;
    }

    sscanf(*line, "%s%n", comp->cmd, &scannedChars);
    logPrint(L_EXTRA, 0, "label = '%s'\n", comp->cmd);
    if (!checkIsLabel(comp->cmd)) {
        logPrint(L_ZERO, 1, "Syntax error in %s:%zu : bad label in jump: '%s'\n",
            comp->inName, comp->lineIdx+1, comp->cmd);
        return false;
    }
    *line += scannedChars;

    label_t *label = findLabel(comp->cmd, &comp->labels);
    if (label != NULL) {
        *comp->ip = label->ip;
        logPrint(L_EXTRA, 0, "\t\tFound in labels, ip = %d\n", label->ip);
        if (label->ip == POISON_IP) {
            jmpLabel_t jmpLabel = {comp->ip, comp->labels.size - 1, comp->lineIdx};
            vectorPush(&comp->fixup,  &jmpLabel);
        }
    } else {
        label_t newLabel = {-1, strdup(comp->cmd)};
        vectorPush(&comp->labels, &newLabel);
        logPrint(L_EXTRA, 0, "\t\tPush in labels, idx = %d, ip = %d, line = %zu\n",
                             comp->labels.size - 1, (int)(comp->ip - comp->code), comp->lineIdx);
        jmpLabel_t jmpLabel = {comp->ip, comp->labels.size - 1, comp->lineIdx};
        vectorPush(&comp->fixup,  &jmpLabel);
    }
    comp->ip++;
    return true;
}

/// @brief  Parse one line
/// Parsers for push, jmp, etc. move ip forward to next command, so in the end of cycle there's no comp->ip++
/// @return true on success, false otherwise
static bool parseCodeLine(compilerData_t *comp) {
    char *line = comp->codeLines[comp->lineIdx];
    #define IP_TO_IDX(comp) ((int)(comp->ip - comp->code))
    int scannedChars = 0;
    logPrint(L_EXTRA, 0, "%s:%d: '%s'\n", comp->inName, comp->lineIdx, line);
    if (checkIsLabel(line))
        return processLabel(comp);

    while (sscanf(line, "%s%n", comp->cmd, &scannedChars) == 1) {
        logPrint(L_EXTRA, 0, "\tBefore scan: '%s'\n", line);
        line += scannedChars;
        logPrint(L_EXTRA, 0, "\tAfter  scan: '%s'\n", line);

        *comp->ip = (int)cmdToEnum(comp->cmd);
        if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
            return false;
        logPrint(L_EXTRA, 0, "\tCmd: `%s` -> %d (ip = %d)\n", comp->cmd, *comp->ip, IP_TO_IDX(comp));

        switch(*comp->ip) {
            case CMD_PUSH:
            {
                //TODO: add '+' between register and number
                if (!scanPushArgs(comp, &line))
                    return false;
                break;
            }
            case CMD_POP:
            {
                sscanf(line, "%s%n", comp->cmd, &scannedChars);
                line += scannedChars;
                comp->ip++;
                *comp->ip = cmdToReg(comp->cmd);
                if (checkSyntaxError(comp, CMD_OPS(*comp->ip)))
                    return false;
                comp->ip++;
                break;
            }
            case CMD_JMP: case CMD_JL:
            case CMD_JA:  case CMD_JAE:
            case CMD_JB:  case CMD_JBE:
            case CMD_JE:  case CMD_JNE:
            case CMD_CALL:
            {
                if (!processJmpLabel(comp, &line))
                    return false;
                break;
            }
            default:
            {
                comp->ip++;
                break;
            }
        }
    }
    return true;
}

/// @brief  Strip line by any characters from separators
/// @return Pointer to new end of the line or NULL
static char *removeCommentFromLine(char *line, const char *separators) {
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
    if (error == CMD_SNTXERR) {
        logPrint(L_ZERO, 1, "Syntax error in %s:%zu : unknown command %s\n",
                comp->inName, comp->lineIdx+1, comp->cmd);
        return true;
    }
    return false;
}


static compilerData_t compilerDataCtor(const char *inName, const char *outName) {
    compilerData_t comp = {0};
    comp.inName  = inName;
    comp.outName = outName;

    comp.labels = vectorCtor(0, sizeof(label_t));
    comp.fixup  = vectorCtor(0, sizeof(jmpLabel_t));

    memset(&comp.cmd, 0, MAX_CMD_SIZE);
    comp.code = (int *) calloc(MAX_CODE_SIZE, sizeof(int));
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
    size_t codeSize = size_t(comp->ip - comp->code);
    FILE *outFile = fopen(comp->outName, "wb");
    if (!outFile) return false;
    fprintf(outFile, "%s ", CPU_SIGNATURE);
    fprintf(outFile, "%d %zu\n", CPU_CMD_VERSION, codeSize);
    for (size_t idx = 0; idx < codeSize; idx++) {
        fprintf(outFile, "%x ", comp->code[idx]);
    }
    fclose(outFile);
    return true;
}

bool compile(const char *inName, const char *outName) {
    compilerData_t comp = compilerDataCtor(inName, outName);

    while(comp.lineIdx < comp.lineCnt) {
        if (!parseCodeLine(&comp)) {
            compilerDataDtor(&comp);
            return false;
        }
        comp.lineIdx++;
    }

    for (size_t idx = 0; idx < comp.fixup.size; idx++) {
        jmpLabel_t *label = (jmpLabel_t *)vectorGet(&comp.fixup, idx);
        size_t codeIdx = (size_t) (label->ip - comp.code);
        comp.code[codeIdx] = ((label_t *)vectorGet(&comp.labels, label->index))->ip;
        if (comp.code[codeIdx] == POISON_IP) {
            logPrint(L_ZERO, 1, "Undefined label at %s:%zu : '%s'\n",
                comp.inName, label->lineIdx+1, ((label_t *)vectorGet(&comp.labels, label->index))->label);
            compilerDataDtor(&comp);
            return false;
        }
    }

    bool compilationResult = writeCodeToFile(&comp);
    compilerDataDtor(&comp);
    return compilationResult;
}
