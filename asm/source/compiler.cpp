#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "cpuCommands.h"
#include "error_debug.h"
#include "utils.h"
#include "logger.h"
#include "compiler.h"


static int cmdToReg(const char *cmd);
static enum CMD_OPS cmdToEnum(const char *cmd);
static int scanPushArgs(int *code, FILE* inFile, char * cmd);

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

static int scanPushArgs(int *code, FILE* inFile, char * cmd) {
    int ipMove = 0;

    if (fscanf(inFile, "%d", &code[1]) == 1) {
        *code |= MASK_IMMEDIATE;
        ipMove++;
    } else {
        fscanf(inFile, "%s", cmd);
        *code |= MASK_REGISTER;
        ipMove++;
        code[1] = cmdToReg(cmd);
        if (code[1] == CMD_SNTXERR)
            return CMD_SNTXERR;
        if (fscanf(inFile, "%d", &code[2]) == 1) {
            *code |= MASK_IMMEDIATE;
            ipMove++;
        }
    }
    return ipMove;
}

bool compile(const char *inName, const char *outName) {
    char cmd[50] = "";
    int *code = (int *) calloc(MAX_CODE_SIZE, sizeof(int));
    size_t ip = 0;
    size_t lineCnt = 1;
    FILE* inFile = fopen(inName, "rb");

    #define CHECK_SNTXERR(err)                                                                              \
        if (err == CMD_SNTXERR) {                                                                           \
            free(code);                                                                                     \
            fclose(inFile);                                                                                 \
            logPrint(L_ZERO, 1, "Syntax error in %s:%zu : unknown command %s\n", inName, lineCnt, cmd);     \
            return false;                                                                                   \
        }

    while (fscanf(inFile, "%s", cmd) != EOF) {
        logPrint(L_EXTRA, 0, "ip = %zu, Parsing cmd: `%s`\n", ip, cmd);
        code[ip] = cmdToEnum(cmd);
        CHECK_SNTXERR(code[ip]);
        logPrint(L_EXTRA, 0, "Cmd number: `%d`\n", code[ip]);
        switch(code[ip]) {
        case CMD_PUSH: {
            //TODO: add '+' between register and number
            int argCnt = scanPushArgs(&code[ip], inFile, cmd);
            CHECK_SNTXERR(argCnt);
            ip += argCnt;
            break;
        }
        case CMD_JMP:
        case CMD_JA: {
            if (fscanf(inFile, "%d", &code[ip+1]) != 1) {
                CHECK_SNTXERR(CMD_SNTXERR);
            }
            ip++;
            break;
        }
        case CMD_POP: {
            fscanf(inFile, "%s", cmd);
            ip++;
            code[ip] = cmdToReg(cmd);
            CHECK_SNTXERR(code[ip]);
            break;
        }
        default:
            break;
        }
        lineCnt++;
        ip++;
    }
    #undef CHECK_SNTXERR
    fclose(inFile);
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
