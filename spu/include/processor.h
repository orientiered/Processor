#ifndef PROCESSOR_H
#define PROCESSOR_H

const size_t RAM_SIZE = 121;
const size_t DRAW_WIDTH = 11;
typedef struct {
    size_t size;
    Stack_t stk;
    Stack_t callStk;
    int *code;
    int *ip;
    int regs[CPU_REGS_COUNT + 1];
    int ram[RAM_SIZE];
} cpu_t;

bool cpuCtor(cpu_t *cpu, const char *program);
bool cpuDtor(cpu_t *cpu);

bool cpuRun(cpu_t *cpu);
bool cpuDump(cpu_t *cpu);



#endif
