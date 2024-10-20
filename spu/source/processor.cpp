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

static int* getArgs(cpu_t *cpu) {
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
    if ((command == CMD_PUSH || command == CMD_SLEEP) && !MEMORY) {
        cpu->regs[R0X] = temp;
        if (result) cpu->regs[R0X] += *result;
        result = &cpu->regs[R0X];
    }
    if (command == CMD_POP && REGISTER && !MEMORY && !IMMEDIATE) {
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
        logPrint(L_DEBUG, 0, " %3x", idx);
    }
    logPrint(L_DEBUG, 0, "\n");
    for (size_t idx = startPos; idx < endPos; idx++) {
        logPrint(L_DEBUG, 0, " %3x", cpu->code[idx]);
    }
    logPrint(L_DEBUG, 0, "\n");
    for (size_t spaceCnt = 0; spaceCnt < (curPosition-startPos)*4 + 3; spaceCnt++)
        logPrint(L_DEBUG, 0, " ");

    logPrint(L_DEBUG, 0, "^ %s\n", enumToCmd(CMD_OPS(*cpu->ip & MASK_CMD)));
    logPrint(L_DEBUG, 0, "Stack: ");
    // logPrint(L_DEBUG, 0, "ip = %zu, stk.size = %zu\n", ip, stackGetSize(&cpu->stk));
    for (size_t idx = 0; idx < stackGetSize(&cpu->stk); idx++)
        logPrint(L_DEBUG, 0, "%d ", cpu->stk.data[idx]);
    logPrint(L_DEBUG, 0, "\n");

    logPrint(L_DEBUG, 0, "REGISTERS: rax = %d, rbx = %d, rcx = %d, rdx = %d, rex = %d\n", cpu->regs[RAX], cpu->regs[RBX], cpu->regs[RCX], cpu->regs[RDX], cpu->regs[REX]);
    logPrint(L_DEBUG, 0, "------------------------------------------------\n\n");

    return true;
}

static bool drawRAM(cpu_t *cpu) {
    MY_ASSERT(cpu, abort());
    for (size_t row = 0; row < DRAW_HEIGHT; row++) {
        for (size_t col = 0; col < DRAW_WIDTH; col++) {
            char c = (cpu->ram[row * DRAW_WIDTH + col] == 0) ? '.' : '*';
            putchar(c);
            //putchar(' '); //better width/height ratio
        }
        putchar('\n');
    }
    return true;
}

static bool handleJumps(cpu_t *cpu) {
    MY_ASSERT(cpu, abort());
    bool conditionalJump = false;
    switch(*cpu->ip & MASK_CMD) {
        case CMD_JMP: {
            cpu->ip = cpu->code + cpu->ip[CMD_LEN];
            break;
        }
        case CMD_CALL: {
            //pushing pointer to NEXT command
            stackPush(&cpu->callStk, (size_t) (cpu->ip - cpu->code) + CMD_LEN + ARG_LEN);
            cpu->ip = cpu->code + cpu->ip[CMD_LEN];
            break;
        }
        case CMD_RET: {
            if (stackGetSize(&cpu->callStk) == 0) {
                logPrint(L_ZERO, 1, "Can't return, callStack is empty\n");
                return false;
            }
            cpu->ip = cpu->code + stackPop(&cpu->callStk);
            break;
        }
        case CMD_JL: {
            if (rand() % 2) {
                cpu->ip = cpu->code + cpu->ip[CMD_LEN];
            } else
                cpu->ip += CMD_LEN + ARG_LEN;
            break;
        }
        default: {
            conditionalJump = true;
            break;
        }

    }
    logPrint(L_EXTRA, 0, "conditional = %d\n", conditionalJump);
    if (!conditionalJump)
        return true;

    int val1 = stackPop(&cpu->stk);
    int val2 = stackPop(&cpu->stk);
    bool makeJump = false;
    switch(*cpu->ip & MASK_CMD) {
        case CMD_JA: {
            makeJump = (val1 >  val2);
            break;
        }
        case CMD_JAE: {
            makeJump = (val1 >= val2);
            break;
        }
        case CMD_JB: {
            makeJump = (val1 <  val2);
            break;
        }
        case CMD_JBE: {
            makeJump = (val1 <= val2);
            break;
        }
        case CMD_JE: {
            makeJump = (val1 == val2);
            break;
        }
        case CMD_JNE: {
            makeJump = (val1 != val2);
            break;
        }
        default: {
            logPrint(L_ZERO, 1, "Unknown jump %d\n", *cpu->ip);
            return false;
        }

    }

    if (makeJump)
        cpu->ip = cpu->code + cpu->ip[CMD_LEN];
    else
        cpu->ip += CMD_LEN + ARG_LEN;

    return true;
}

static bool handleMath(cpu_t *cpu) {
    bool twoOperands = false;

    int val1 = stackPop(&cpu->stk);
    switch(*cpu->ip & MASK_CMD) {
        case CMD_SQRT: {
            stackPush(&cpu->stk, (int)sqrt(val1));
            break;
        }
        case CMD_SIN: {
            stackPush(&cpu->stk, (int)sin(val1));
            break;
        }
        case CMD_COS: {
            stackPush(&cpu->stk, (int)cos(val1));
            break;
        }
        default: {
            twoOperands = true;
            break;
        }
    }

    if (!twoOperands) {
        cpu->ip += CMD_LEN;
        return true;
    }

    int val2 = stackPop(&cpu->stk);
    switch(*cpu->ip & MASK_CMD) {
        case CMD_ADD: {
            stackPush(&cpu->stk, val1 + val2);
            break;
        }
        case CMD_SUB: {
            stackPush(&cpu->stk, val2 - val1);
            break;
        }
        case CMD_MUL: {
            stackPush(&cpu->stk, val1 * val2);
            break;
        }
        case CMD_DIV: {
            stackPush(&cpu->stk, val2 / val1);
            break;
        }
        default: {
            logPrint(L_ZERO, 1, "Unknown math command %d\n", *cpu->ip);
            return false;
        }
    }
    cpu->ip += CMD_LEN;
    return true;
}

bool cpuRun(cpu_t *cpu) {
    bool run = true;
    logPrintWithTime(L_DEBUG, 0, "Entered cpuRun\n");
    struct timeval startTime = {0}, currentTime = {0};
    gettimeofday(&startTime, NULL);
    while (run) {
        cpuDump(cpu);
        switch(*cpu->ip & MASK_CMD) {
            case CMD_PUSH: {
                stackPush(&cpu->stk, *getArgs(cpu));
                break;
            }
            case CMD_POP: {
                *getArgs(cpu) = stackPop(&cpu->stk);
                break;
            }
            case CMD_ADD:  case CMD_SUB:
            case CMD_MUL:  case CMD_DIV:
            case CMD_SIN:  case CMD_COS:
            case CMD_SQRT:
            {
                if (!handleMath(cpu))
                    return false;
                break;
            }
            case CMD_SLEEP: {
                //sleeping *getArgs(cpu) ms
                usleep(*getArgs(cpu) * 1000);
                break;
            }
            case CMD_JMP:  case CMD_JL:
            case CMD_CALL: case CMD_RET:
            case CMD_JA:   case CMD_JAE:
            case CMD_JB:   case CMD_JBE:
            case CMD_JE:   case CMD_JNE:
            {
                if (!handleJumps(cpu))
                    return false;
                break;
            }
            case CMD_TIME: {
                gettimeofday(&currentTime, NULL);
                int timeInMs = ((currentTime.tv_sec  - startTime.tv_sec) * 1000000 +
                                (currentTime.tv_usec - startTime.tv_usec)) / 1000;
                stackPush(&cpu->stk, timeInMs);
                cpu->ip += CMD_LEN;
                break;
            }
            case CMD_OUT: {
                int val = stackPop(&cpu->stk);
                printf("%d\n", val);
                cpu->ip += CMD_LEN;
                break;
            }
            case CMD_IN: {
                int val = 0;
                scanf("%d", &val);
                stackPush(&cpu->stk, val);
                cpu->ip += CMD_LEN;
                break;
            }
            case CMD_DRAW: {
                drawRAM(cpu);
                cpu->ip += CMD_LEN;
                break;
            }
            case CMD_DRAWR: {
                drawRAM(cpu);
                cpu->ip += CMD_LEN;
                printf(RETURN_ESCAPE_SEQUENCE);
                fflush(stdout);
                break;
            }
            case CMD_DUMP: {
                stackDump(&cpu->stk);
                cpu->ip += CMD_LEN;
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


