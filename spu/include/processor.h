#ifndef PROCESSOR_H
#define PROCESSOR_H

typedef struct {
    size_t size;
    Stack_t stk;
    int *code;
} cpu_t;

enum CMD_OPS {
    CMD_SNTXERR = -1,
    CMD_PUSH = 1,
    CMD_ADD,
    CMD_SUB,
    CMD_MUL,
    CMD_DIV,
    CMD_IN,
    CMD_OUT,
    CMD_HLT,
};

cpu_t cpuCtor(const char *program);
bool cpuDtor(cpu_t *cpu);

bool cpuRun(cpu_t *cpu);



#endif
