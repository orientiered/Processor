#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "error_debug.h"
#include "utils.h"
#include "logger.h"
#include "cStack.h"
#include "processor.h"

static const size_t startStackSize = 5;

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

cpu_t cpuCtor(const char *program) {
    FILE *programFile = fopen(program, "r");
    if (!programFile) {
        logPrint(L_ZERO, 1, "Failed to read file\n");
    }

    cpu_t cpu = {0};

    //fscanf(programFile, "%zu", &cpu.size);
    //logPrint(L_DEBUG, 0, "Code size = %zu\n", cpu.size);
    // for (size_t ip = 0; ip < size; ip++) {
    //     fscanf(programFile, "%d", &code[ip]);
    // }
    size_t startCodeSize = 1000;
    int *code = (int *) calloc(startCodeSize, sizeof(int));
    char cmd[50] = "";
    size_t ip = 0;
    while (fscanf(programFile, "%s", cmd) != EOF) {
        logPrint(L_EXTRA, 0, "ip = %zu, Parsing cmd: `%s`\n", ip, cmd);
        code[ip] = cmdToEnum(cmd);
        logPrint(L_EXTRA, 0, "Cmd number: `%d`\n", code[ip]);
        if (code[ip] == CMD_PUSH) {
            ip++;
            fscanf(programFile, "%d", &code[ip]);
        }
        ip++;
    }
    cpu.code = code;
    cpu.size = ip;
    fclose(programFile);
    stackCtor(&cpu.stk, startStackSize);
    return cpu;
}

bool cpuDtor(cpu_t *cpu) {
    if (cpu == NULL) return false;
    cpu->size = 0;
    stackDtor(&cpu->stk);
    free(cpu->code); cpu->code = NULL;
    return true;
}

bool cpuRun(cpu_t *cpu) {
    bool run = true;
    size_t ip = 0;
    logPrintWithTime(L_DEBUG, 0, "Entered cpuRun\n");
    while (run) {
        logPrintWithTime(L_DEBUG, 0, "ip = %zu, stk.size = %zu\n", ip, stackGetSize(&cpu->stk));
        switch(cpu->code[ip]) {
        case CMD_PUSH:
            stackPush(&cpu->stk, cpu->code[ip+1]);
            ip += 2;
            break;
        case CMD_ADD: {
            int val1 = stackPop(&cpu->stk);
            int val2 = stackPop(&cpu->stk);
            stackPush(&cpu->stk, val1+val2);
            ip++;
            break;
            }
        case CMD_SUB: {
            int val1 = stackPop(&cpu->stk);
            int val2 = stackPop(&cpu->stk);
            stackPush(&cpu->stk, val2 - val1);
            ip++;
            break;
            }
        case CMD_MUL: {
            int val1 = stackPop(&cpu->stk);
            int val2 = stackPop(&cpu->stk);
            stackPush(&cpu->stk, val1 * val2);
            ip++;
            break;
            }
        case CMD_DIV: {
            int val1 = stackPop(&cpu->stk);
            int val2 = stackPop(&cpu->stk);
            stackPush(&cpu->stk, val2 / val1);
            ip++;
            break;
            }
        case CMD_OUT: {
            int val = stackPop(&cpu->stk);
            printf("Result: %d\n", val);
            ip++;
            break;
            }
        case CMD_HLT:
            run = false;
            break;
        default:
            logPrint(L_ZERO, 1, "Invalid command code: %d\n", cpu->code[ip]);
            return false;
            break;
        }
    }
    return true;
}


