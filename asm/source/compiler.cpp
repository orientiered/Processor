#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "error_debug.h"
#include "utils.h"
#include "logger.h"
#include "compiler.h"


static enum CMD_OPS cmdToEnum(const char *cmd);
static enum CMD_OPS cmdToEnum(const char *cmd) {
    typedef struct {
        const char *name;
        enum CMD_OPS op;
    } command_t;
    const command_t commands[] = {
        {"push", CMD_PUSH},
        {"add", CMD_ADD},
        {"sub", CMD_SUB},
        {"mul", CMD_MUL},
        {"div", CMD_DIV},
        {"out", CMD_OUT},
        {"hlt", CMD_HLT}
        //TODO: add more commands
    };
    for (size_t idx = 0; idx < ARRAY_SIZE(commands); idx++) {
        if (strcmp(cmd, commands[idx].name) == 0)
            return commands[idx].op;
    }
    return CMD_SNTXERR;

}

bool compile(const char *inName, const char *outName) {
    char cmd[50] = "";
    int *code = (int *) calloc(MAX_CODE_SIZE, sizeof(int));
    size_t ip = 0;
    FILE* inFile = fopen(inName, "rb");
    while (fscanf(inFile, "%s", cmd) != EOF) {
        logPrint(L_EXTRA, 0, "ip = %zu, Parsing cmd: `%s`\n", ip, cmd);
        code[ip] = cmdToEnum(cmd);
        logPrint(L_EXTRA, 0, "Cmd number: `%d`\n", code[ip]);
        if (code[ip] == CMD_PUSH) {
            ip++;
            fscanf(inFile, "%d", &code[ip]);
        }
        ip++;
    }
    fclose(inFile);
    size_t codeSize = ip;
    FILE *outFile = fopen(outName, "wb");
    fprintf(outFile, "%d ", ip);
    for (size_t idx = 0; idx < codeSize; idx++) {
        fprintf(outFile, "%d ", code[idx]);
    }
    free(code);
    fclose(outFile);
    return true;
}
