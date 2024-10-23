#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#include "error_debug.h"
#include "utils.h"
#include "logger.h"
#include "cStack.h"
#include "cpuCommands.h"
#include "processor.h"

static const size_t startStackSize = 5;


static const char *enumToCmd(enum CMD_OPS cmdCode) {
    for (size_t idx = 0; idx < ARRAY_SIZE(CPU_COMMANDS_ARRAY); idx++) {
        if (CPU_COMMANDS_ARRAY[idx].op == cmdCode)
            return CPU_COMMANDS_ARRAY[idx].name;
    }
    return "Unknown command";
}

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

bool cpuCtor(cpu_t *cpu, const char *program) {
    FILE *programFile = fopen(program, "r");
    if (!programFile) {
        logPrint(L_ZERO, 1, "Failed to read file\n");
        return false;
    }

    if (!readHeader(programFile, &cpu->size))
        return false;

    cpu->code = (int *) calloc(cpu->size, sizeof(int));
    cpu->ram  = (int *) calloc(RAM_SIZE,  sizeof(int));
    cpu->ip = cpu->code;
    logPrint(L_DEBUG, 0, "Code size = %zu\n", cpu->size);

    if (fread(cpu->code, sizeof(int), cpu->size, programFile) != cpu->size) {
        logPrint(L_ZERO, 1, "Failed to read file\n");
        return false;
    }
    // for (size_t ip = 0; ip < cpu->size; ip++) {
    //     fscanf(programFile, "%x", &cpu->code[ip]);
    // }
    fclose(programFile);

    stackCtor(&cpu->stk,     startStackSize);
    stackCtor(&cpu->callStk, startStackSize);
    for (size_t i = 0; i < CPU_REGS_COUNT + 1; i++)
        cpu->regs[i] = 0;
    return true;
}

bool cpuDtor(cpu_t *cpu) {
    if (cpu == NULL) return false;
    cpu->size = 0;
    stackDtor(&cpu->stk);
    stackDtor(&cpu->callStk);
    free(cpu->code); cpu->code = NULL;
    free(cpu->ram) ; cpu->ram  = NULL;
    return true;
}

// argType = 0 push-like command
//         = 1 pop--like command
static int* getArgs(cpu_t *cpu, int argType) {
    MY_ASSERT(cpu, abort());
    //MY_ASSERT((*cpu->ip & MASK_CMD) == CMD_PUSH || (*cpu->ip & MASK_CMD) == CMD_POP, abort());

    int* result    = NULL;
    int  temp      = 0;
    int  command   = *(cpu->ip) & MASK_CMD;
    bool REGISTER  = *(cpu->ip) & MASK_REGISTER;
    bool IMMEDIATE = *(cpu->ip) & MASK_IMMEDIATE;
    bool MEMORY    = *(cpu->ip) & MASK_MEMORY;
    cpu->ip += CMD_LEN;

    if (REGISTER) {
        result   = &cpu->regs[*cpu->ip];
        cpu->ip += REG_LEN;
    }
    if (IMMEDIATE) {
        temp    += *cpu->ip;
        cpu->ip += ARG_LEN;
    }
    if (MEMORY) {
        if (result != NULL)
            result = cpu->ram + *result + temp;
        else
            result = cpu->ram + temp;
    }

    logPrint(L_EXTRA, 0, "operation %x, \ttemp = %d\n", command, temp);
    if (argType == 0 && !MEMORY) {
        cpu->regs[R0X] = temp;
        if (result) cpu->regs[R0X] += *result;
        result = &cpu->regs[R0X];
    }
    if (argType == 1 && REGISTER && !MEMORY && !IMMEDIATE) {
        logPrint(L_EXTRA, 0, "\tpopping to register\n");
    }

    return result;
}

bool cpuDump(cpu_t *cpu) {
    if (getLogLevel() < L_DEBUG) return true;
    logPrint(L_DEBUG, 0, "\n------------------CPU DUMP----------------------\n");
    size_t curPosition = ((size_t)(cpu->ip - cpu->code));
    size_t startPos = (curPosition > 10) ? curPosition - 10 : 0;
    size_t endPos = (curPosition + 10 < cpu->size) ? curPosition + 10 : cpu->size;
    for (size_t idx = startPos; idx <= endPos; idx++) {
        logPrint(L_DEBUG, 0, " %7x", idx);
    }
    logPrint(L_DEBUG, 0, "\n");
    for (size_t idx = startPos; idx < endPos; idx++) {
        logPrint(L_DEBUG, 0, " %7x", cpu->code[idx]);
    }
    logPrint(L_DEBUG, 0, "\n");
    for (size_t spaceCnt = 0; spaceCnt < (curPosition-startPos)*8 + 7; spaceCnt++)
        logPrint(L_DEBUG, 0, " ");

    logPrint(L_DEBUG, 0, "^ %s\n", enumToCmd(CMD_OPS(*cpu->ip & MASK_CMD)));
    logPrint(L_DEBUG, 0, "Stack: ");
    // logPrint(L_DEBUG, 0, "ip = %zu, stk.size = %zu\n", ip, stackGetSize(&cpu->stk));
    for (size_t idx = 0; idx < stackGetSize(&cpu->stk); idx++)
        logPrint(L_DEBUG, 0, "%d ", cpu->stk.data[idx]);
    logPrint(L_DEBUG, 0, "\n");

    logPrint(L_DEBUG, 0, "CallStack: ");
    for (size_t idx = 0; idx < stackGetSize(&cpu->callStk); idx++)
        logPrint(L_DEBUG, 0, "0x%X ", cpu->callStk.data[idx]);
    logPrint(L_DEBUG, 0, "\n");

    logPrint(L_DEBUG, 0, "REGISTERS: rax = %d, rbx = %d, rcx = %d, rdx = %d, rex = %d\n", cpu->regs[RAX], cpu->regs[RBX], cpu->regs[RCX], cpu->regs[RDX], cpu->regs[REX]);
    logPrint(L_DEBUG, 0, "------------------------------------------------\n\n");

    return true;
}

static bool drawRAM(cpu_t *cpu, const size_t height, const size_t width) {
    MY_ASSERT(cpu, abort());
    for (size_t row = 0; row < height; row++) {
        for (size_t col = 0; col < width; col++) {
            char c = (cpu->ram[row * width + col] == 0) ? WHITE_CHAR : BLACK_CHAR;
            putchar(c);
            //putchar(' '); //better width/height ratio
        }
        putchar('\n');
    }
    return true;
}

#define DEF_CMD_(cmdName, cmdIndex, argHandler, ...) \
    case CMD_##cmdName: __VA_ARGS__; break;

bool cpuRun(cpu_t *cpu) {
    bool run = true;
    logPrintWithTime(L_DEBUG, 0, "Entered cpuRun\n");
    struct timeval startTime = {0}, currentTime = {0};
    gettimeofday(&startTime, NULL);
    while (run) {
        cpuDump(cpu);
        switch(*cpu->ip & MASK_CMD) {
            #include "Commands.h"
            default: {
                logPrint(L_ZERO, 1, "Invalid command code '%d' at 0x%X\n", *cpu->ip, size_t(cpu->ip - cpu->code));
                return false;
            }
        }
    }
    return true;
}
#undef DEF_CMD_


