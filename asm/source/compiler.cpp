#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "cpuCommands.h"
#include "error_debug.h"
#include "utils.h"
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

static int scanPushArgs(int *code, char **line, char * cmd) {
    int ipMove = 0;

    int scannedChars = 0;
    if (sscanf(*line, "%d%n", code + 1, &scannedChars) == 1) {
        *line += scannedChars;
        *code |= MASK_IMMEDIATE;
        ipMove++;
    } else {
        sscanf(*line, "%s%n", cmd, &scannedChars);
        *line += scannedChars;
        *code |= MASK_REGISTER;
        ipMove++;
        code[1] = cmdToReg(cmd);
        if (code[1] == CMD_SNTXERR)
            return CMD_SNTXERR;
        if (sscanf(*line, "%d%n", code + 2, &scannedChars) == 1) {
            *line += scannedChars;
            *code |= MASK_IMMEDIATE;
            ipMove++;
        }
    }
    return ipMove;
}

static int parseCodeLine(char *line, int *code, char *cmd) {
    int ip = 0;
    int scannedChars = 0;
    logPrint(L_EXTRA, 0, "---Parsing line '%s'\n", line);
    while (sscanf(line, "%s%n", cmd, &scannedChars) == 1) {
        logPrint(L_EXTRA, 0, "line before move: '%s'\n", line);
        line += scannedChars;
        logPrint(L_EXTRA, 0, "line after  move: '%s'\n", line);
        logPrint(L_EXTRA, 0, "ip = %d, Parsing cmd: `%s`\n", ip, cmd);
        code[ip] = cmdToEnum(cmd);
        if (code[ip] == CMD_SNTXERR)
            return CMD_SNTXERR;
        logPrint(L_EXTRA, 0, "Cmd number: `%d`\n", code[ip]);

        switch(code[ip]) {
            case CMD_PUSH: {
                //TODO: add '+' between register and number
                int argCnt = scanPushArgs(&code[ip], &line, cmd);
                logPrint(L_EXTRA, 0, "Scanned push args: %d\n", argCnt);
                if (argCnt == CMD_SNTXERR)
                    return CMD_SNTXERR;
                ip += argCnt;
                break;
            }
            case CMD_JMP:
            case CMD_JA: {
                if (sscanf(line, "%d%n", &code[ip+1], &scannedChars) != 1)
                    return CMD_SNTXERR;
                line += scannedChars;
                ip++;
                break;
            }
            case CMD_POP: {
                sscanf(line, "%s%n", cmd, &scannedChars);
                line += scannedChars;
                ip++;
                code[ip] = cmdToReg(cmd);
                if (code[ip] == CMD_SNTXERR)
                    return CMD_SNTXERR;
                break;
            }
            default:
                break;
        }
        ip++;
    }
    return ip;
}

bool compile(const char *inName, const char *outName) {
    char cmd[50] = "";
    int *code = (int *) calloc(MAX_CODE_SIZE, sizeof(int));
    size_t ip = 0;

    size_t lineCnt = 0;
    char **codeLines = readLinesFromFile(inName, &lineCnt);
    for (size_t idx = 0; idx < lineCnt; idx++) {
        logPrint(L_EXTRA, 0, "%s\n", codeLines[idx]);
    }
    #define CHECK_SNTXERR(err)                                                                              \
        if (err == CMD_SNTXERR) {                                                                           \
            free(code);                                                                                     \
            free(codeLines[0]);                                                                             \
            free(codeLines);                                                                                \
            logPrint(L_ZERO, 1, "Syntax error in %s:%zu : unknown command %s\n", inName, lineIdx+1, cmd);   \
            return false;                                                                                   \
        }
    for (size_t lineIdx = 0; lineIdx < lineCnt; lineIdx++) {
        //erasing comments: they start with ;
        char *semicolon = strchr(codeLines[lineIdx], ';');
        if (semicolon != NULL) *semicolon = '\0';

        int ipMove = parseCodeLine(codeLines[lineIdx], code + ip, cmd);
        CHECK_SNTXERR(ipMove);
        ip += ipMove;
    }

    #undef CHECK_SNTXERR
    free(codeLines[0]);
    free(codeLines);
    size_t codeSize = ip;
    FILE *outFile = fopen(outName, "wb");
    fprintf(outFile, "%s ", CPU_SIGNATURE);
    fprintf(outFile, "%d %zu\n", CPU_CMD_VERSION, ip);
    for (size_t idx = 0; idx < codeSize; idx++) {
        fprintf(outFile, "%x ", code[idx]);
    }
    free(code);
    fclose(outFile);
    return true;
}
