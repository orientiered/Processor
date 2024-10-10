#ifndef PROCESSOR_H
#define PROCESSOR_H

typedef struct {
    size_t size;
    Stack_t stk;
    int *code;
} cpu_t;

cpu_t cpuCtor(const char *program);
bool cpuDtor(cpu_t *cpu);

bool cpuRun(cpu_t *cpu);



#endif
