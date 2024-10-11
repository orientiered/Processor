#ifndef PROCESSOR_H
#define PROCESSOR_H

typedef struct {
    size_t size;
    Stack_t stk;
    int *code;
    int *ip;
    int regs[CPU_REGS_COUNT + 1];
} cpu_t;

bool cpuCtor(cpu_t *cpu, const char *program);
bool cpuDtor(cpu_t *cpu);

bool cpuRun(cpu_t *cpu);
bool cpuDump(cpu_t *cpu);



#endif
