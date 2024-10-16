#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
    typedef struct {
        const char *name;
        enum CMD_OPS op;
    } command_t;
    const command_t commands[] = {
        {"push" ,   CMD_PUSH    },
        {"pop"  ,   CMD_POP     },
        {"add"  ,   CMD_ADD     },
        {"sub"  ,   CMD_SUB     },
        {"mul"  ,   CMD_MUL     },
        {"div"  ,   CMD_DIV     },
        {"sqrt" ,   CMD_SQRT    },
        {"jmp"  ,   CMD_JMP     },
        {"ja"   ,   CMD_JA      },
        {"in"   ,   CMD_IN      },
        {"out"  ,   CMD_OUT     },
        {"dump" ,   CMD_DUMP    },
        {"hlt"  ,   CMD_HLT     }
    };
    for (size_t idx = 0; idx < ARRAY_SIZE(commands); idx++) {
        if (strcmp(cmd, commands[idx].name) == 0)
            return commands[idx].op;
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
        *comp->ip = cmdToReg(cmd);
        if (checkSyntaxError(comp, *comp->ip))
            return false;
        comp->ip++;
        if (sscanf(*line, "%d%n", comp->ip, &scannedChars) == 1) {
            *line += scannedChars;
            *pushIp |= MASK_IMMEDIATE;
        }
    }
    logPrint(L_EXTRA, 0, "__Scanned %d push arguments\n", (int)(comp->ip - pushIp));
    return true;
}

static bool checkIfLabel(char *line, char *cmd) {
    //label <=> no spaces, only one word, ends with :
    size_t len = 0;
    sscanf(line, "%[^ :\n]%n", cmd, &len);
    if (!(strlen(line) > len && line[len+1] == ':'))
        return false;
    line += len + 1;
    sscanf(line, "%s%n", cmd, &len);
    if (cmd[0] == '\0') return true;
}

static label_t *findLabel(char *cmd, Vector_t* labels) {
    for (char *idx = (char *)labels->base; idx <= (char *)vectorGet(labels->base, labels->size - 1); idx += labels->elemSize) {
        if (strcmp(cmd, ((label_t *)idx)->label) == 0)
            return (label_t *)idx;
    }
    return (label_t *)NULL;
}

static bool processLabel(compilerData_t *comp) {

}

static bool processJmpLabel(compilerData_t *comp) {
    MY_ASSERT(comp, abort());
    logPrint(L_EXTRA, 0, "Parsing jmp label %s:", comp->cmd);
    label_t *label = findLabel(comp->cmd, comp->labels);
    if (label != NULL) {
        *comp->ip = label->ip;
        logPrint(L_EXTRA, 0, "found in labels, ip = %d\n", label->ip);
        if (label->ip == POISON_IP) {
            jmpLabel_t jmpLabel = {comp->ip, comp->labels.size - 1};
            vectorPush(&comp->fixup,  &jmpLabel);
        }
    } else {
        label_t label = {-1, strdup(comp->cmd)};
        vectorPush(&comp->labels, &label);
        logPrint(L_EXTRA, 0, "push in labels, idx = %d, ip = %d\n",
                             comp->labels.size - 1, (int)(comp->ip - comp->code));
        jmpLabel_t jmpLabel = {comp->ip, comp->labels.size - 1};
        vectorPush(&comp->fixup,  &jmpLabel);
    }
    return true;
}

/// @brief  Parse one line
/// @return true on success, false otherwise
static bool parseCodeLine(compilerData_t *comp) {
    char *line = comp->codeLines[comp->lineIdx];
    #define IP_TO_IDX(comp) ((int)(comp->ip - comp->code))
    int scannedChars = 0;
    logPrint(L_EXTRA, 0, "---Parsing line '%s'\n", line);
    if (checkIfLabel(line, comp->cmd))
        return processLabel(comp);

    while (sscanf(line, "%s%n", comp->cmd, &scannedChars) == 1) {
        logPrint(L_EXTRA, 0, "-line before scan: '%s'\n", line);
        line += scannedChars;
        logPrint(L_EXTRA, 0, "-line after  scan: '%s'\n", line);
        logPrint(L_EXTRA, 0, "--ip = %d, Parsing cmd: `%s`\n", IP_TO_IDX(comp), comp->cmd);

        code[ip] = cmdToEnum(cmd);
        if (checkSyntaxError(comp, *comp->ip)
            return false;
        logPrint(L_EXTRA, 0, "Cmd number: `%d`\n", code[ip]);

        switch(comp->ip) {
            case CMD_PUSH: {
                //TODO: add '+' between register and number
                if (!scanPushArgs(comp, line))
                    return false;
                break;
            }
            case CMD_JMP:
            case CMD_JA: {
                bool isTextLabel = false;
                comp->ip++;
                if (sscanf(line, "%d%n", comp->ip, &scannedChars) != 1) {
                    sscanf(line, "%s%n", comp->cmd, &scannedChars);
                    isTextLabel = true;
                }
                line += scannedChars;
                if (isTextLabel)
                    processJmpLabel(comp);
                break;
            }
            case CMD_POP: {
                sscanf(line, "%s%n", cmd, &scannedChars);
                line += scannedChars;
                comp->ip++;
                *comp->ip = cmdToReg(cmd);
                if (checkSyntaxError(comp, *comp->ip)
                    return false;
                break;
            }
            default:
                break;
        }
        comp->ip++;
    }
    return true;
}

/// @brief  Strip line by any characters from separators
/// @return Pointer to new end of the line or NULL
static char *removeCommentFromLine(char *line, const char *separators) {
    for (char *symbol = line; symbol != '\0'; symbol++) {
        for (char *sep = separators; *sep != '\0'; sep++) {
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

    comp.cmd = "";
    comp.code = (int *) calloc(MAX_CODE_SIZE, sizeof(int));
    comp.ip = code;

    comp.codeLines = readLinesFromFile(inName, &comp.lineCnt);
    logPrint(L_EXTRA, 0, "------Program text---------\n");
    for (size_t idx = 0; idx < comp.lineCnt; idx++)
        logPrint(L_EXTRA, 0, "%s\n", codeLines[idx]);
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
        //erasing comments: they start with ;
        removeCommentFromLine(comp.codeLines[comp.lineIdx], ';#');

        if (!parseCodeLine(&cmp)) {
            compilerDataDtor(&comp);
            return false;
        }
        comp.lineIdx++;
    }

    bool compilationResult = writeCodeToFile(&cmp);
    compilerDataDtor(&comp);
    return compilationResult;
}
