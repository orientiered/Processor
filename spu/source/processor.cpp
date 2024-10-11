#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "error_debug.h"
#include "utils.h"
#include "logger.h"
#include "cStack.h"
#include "cpuCommands.h"
#include "processor.h"

static const size_t startStackSize = 5;


bool cpuCtor(cpu_t *cpu, const char *program) {
    FILE *programFile = fopen(program, "r");
    if (!programFile) {
        logPrint(L_ZERO, 1, "Failed to read file\n");
        return false;
    }

    char signature[50] = "";
    fscanf(programFile, "%s", signature);
    if (strcmp(signature, CPU_SIGNATURE) != 0) {
        logPrint(L_ZERO, 1, "Wrong code signature: expected '%s', got '%s'\n", CPU_SIGNATURE, signature);
        return false;
    }

    int cmdVersion = 0;
    fscanf(programFile, "%d", &cmdVersion);
    if (cmdVersion != CPU_CMD_VERSION) {
        logPrint(L_ZERO, 1, "Wrong command set version: expected %d, got %d\n", CPU_CMD_VERSION, cmdVersion);
        return false;
    }

    fscanf(programFile, "%zu", &cpu->size);
    cpu->code = (int *) calloc(cpu->size, sizeof(int));
    cpu->ip = cpu->code;
    logPrint(L_DEBUG, 0, "Code size = %zu\n", cpu->size);
    for (size_t ip = 0; ip < cpu->size; ip++) {
        fscanf(programFile, "%x", &cpu->code[ip]);
    }
    fclose(programFile);

    stackCtor(&cpu->stk, startStackSize);
    for (int i = 0; i < CPU_REGS_COUNT + 1; i++)
        cpu->regs[i] = 0;
    return true;
}

bool cpuDtor(cpu_t *cpu) {
    if (cpu == NULL) return false;
    cpu->size = 0;
    stackDtor(&cpu->stk);
    free(cpu->code); cpu->code = NULL;
    return true;
}

static int getPushArg(cpu_t *cpu) {
    MY_ASSERT((*cpu->ip & 0x0F) == CMD_PUSH, abort());
    int result = 0;
    int cmd = *(cpu->ip);
    if (cmd & MASK_REGISTER) {
        cpu->ip++;
        result += cpu->regs[*cpu->ip];
    }
    if (cmd & MASK_IMMEDIATE) {
        cpu->ip++;
        result += *cpu->ip;
    }
    return result;
}

bool cpuDump(cpu_t *cpu) {
    logPrint(L_DEBUG, 0, "------------------CPU DUMP----------------------\n");
    size_t curPosition = ((size_t)(cpu->ip - cpu->code));
    size_t startPos = (curPosition > 10) ? curPosition - 10 : 0;
    size_t endPos = (curPosition + 10 < cpu->size) ? curPosition + 10 : cpu->size;
    for (size_t idx = startPos; idx <= endPos; idx++) {
        logPrint(L_DEBUG, 0, " %3x", idx);
    }
    logPrint(L_DEBUG, 0, "\n");
    for (size_t idx = startPos; idx < endPos; idx++) {
        logPrint(L_DEBUG, 0, " %3x", cpu->code[idx]);
    }
    logPrint(L_DEBUG, 0, "\n");
    for (size_t spaceCnt = 0; spaceCnt < (curPosition-startPos)*4 + 3; spaceCnt++)
        logPrint(L_DEBUG, 0, " ");
    logPrint(L_DEBUG, 0, "^\n");

    // logPrint(L_DEBUG, 0, "ip = %zu, stk.size = %zu\n", ip, stackGetSize(&cpu->stk));
    logPrint(L_DEBUG, 0, "------------------------------------------------\n");

    return true;
}

bool cpuRun(cpu_t *cpu) {
    bool run = true;
    logPrintWithTime(L_DEBUG, 0, "Entered cpuRun\n");
    while (run) {
        cpuDump(cpu);
        switch(*cpu->ip & 0x0F) {
            case CMD_PUSH: {
                stackPush(&cpu->stk, getPushArg(cpu));
                cpu->ip++;
                break;
            }
            case CMD_POP: {
                cpu->ip++;
                cpu->regs[*(cpu->ip)] = stackPop(&cpu->stk);
                cpu->ip++;
                break;
            }
            case CMD_ADD: {
                int val1 = stackPop(&cpu->stk);
                int val2 = stackPop(&cpu->stk);
                stackPush(&cpu->stk, val1+val2);
                cpu->ip++;
                break;
            }
            case CMD_SUB: {
                int val1 = stackPop(&cpu->stk);
                int val2 = stackPop(&cpu->stk);
                stackPush(&cpu->stk, val2 - val1);
                cpu->ip++;
                break;
            }
            case CMD_MUL: {
                int val1 = stackPop(&cpu->stk);
                int val2 = stackPop(&cpu->stk);
                stackPush(&cpu->stk, val1 * val2);
                cpu->ip++;
                break;
            }
            case CMD_DIV: {
                int val1 = stackPop(&cpu->stk);
                int val2 = stackPop(&cpu->stk);
                stackPush(&cpu->stk, val2 / val1);
                cpu->ip++;
                break;
            }
            case CMD_SQRT: {
                int val = stackPop(&cpu->stk);
                stackPush(&cpu->stk, (int)sqrt(val));
                cpu->ip++;
                break;
            }
            case CMD_JMP: {
                cpu->ip = cpu->code + cpu->ip[1];
                break;
            }
            case CMD_JA: {
                int val1 = stackPop(&cpu->stk);
                int val2 = stackPop(&cpu->stk);
                if (val1 > val2)
                    cpu->ip = cpu->code + cpu->ip[1];
                else
                    cpu->ip += 2;
                break;
            }
            case CMD_OUT: {
                int val = stackPop(&cpu->stk);
                printf("Result: %d\n", val);
                cpu->ip++;
                break;
            }
            case CMD_IN: {
                int val = 0;
                scanf("%d", &val);
                stackPush(&cpu->stk, val);
                cpu->ip++;
                break;
            }
            case CMD_DUMP: {
                stackDump(&cpu->stk);
                cpu->ip++;
                break;
            }
            case CMD_HLT: {
                run = false;
                break;
            }
            default: {
                logPrint(L_ZERO, 1, "Invalid command code: %d\n", *cpu->ip);
                return false;
            }
        }
    }
    return true;
}


