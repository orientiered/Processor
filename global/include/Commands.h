#define ARG_NONE      comp->ip += CMD_LEN
#define ARG_PUSH_LIKE scanArgs(comp, &line, 0)
#define ARG_POP_LIKE  scanArgs(comp, &line, 1)
#define ARG_LABEL     scanJmpLabel(comp, &line)

#define PUSH(val)     stackPush(&cpu->stk, val)
#define POP()         stackPop(&cpu->stk)

#define IP            cpu->ip
#define IP_IDX        size_t(cpu->ip - cpu->code)

#define GET_ARG     *getArgs(cpu)
#define MATH_TWO(mathOp)           \
    int a = POP(), b = POP();      \
    PUSH(a mathOp b);              \
    IP += CMD_LEN + 2 * ARG_LEN;   \

#define MATH_ONE(mathFunc)         \
    PUSH(mathFunc(POP()));         \
    IP += CMD_LEN + ARG_LEN;       \

DEF_CMD_(HLT,   0,  ARG_NONE, {
    run = false;
})

DEF_CMD_(PUSH,  1,  ARG_PUSH_LIKE, {
    PUSH(GET_ARG);
})
DEF_CMD_(POP,   2,  ARG_POP_LIKE, {
    GET_ARG = POP();
})

DEF_CMD_(ADD,   3,  ARG_NONE, {MATH_TWO(+)})
DEF_CMD_(SUB,   4,  ARG_NONE, {MATH_TWO(-)
DEF_CMD_(MUL,   5,  ARG_NONE)
DEF_CMD_(DIV,   6,  ARG_NONE)
DEF_CMD_(SQRT,  7,  ARG_NONE)
DEF_CMD_(SIN,   8,  ARG_NONE)
DEF_CMD_(COS,   9,  ARG_NONE)

DEF_CMD_(JMP,   10, ARG_LABEL)
DEF_CMD_(JA,    11, ARG_LABEL)
DEF_CMD_(JAE,   12, ARG_LABEL)
DEF_CMD_(JB,    13, ARG_LABEL)
DEF_CMD_(JBE,   14, ARG_LABEL)
DEF_CMD_(JE,    15, ARG_LABEL)
DEF_CMD_(JNE,   16, ARG_LABEL)
DEF_CMD_(JL,    17, ARG_LABEL)

DEF_CMD_(CALL,  18, ARG_LABEL)
DEF_CMD_(RET,   19, ARG_NONE)
DEF_CMD_(SLEEP, 20, ARG_PUSH_LIKE)
DEF_CMD_(TIME,  21, ARG_NONE)

DEF_CMD_(IN,    22, ARG_NONE)
DEF_CMD_(OUT,   23, ARG_NONE)
DEF_CMD_(DRAW,  24, ARG_NONE)
DEF_CMD_(DRAWR, 25, ARG_NONE)

DEF_CMD_(DUMP,  26, ARG_NONE)

#undef ARG_NONE
#undef ARG_PUSH_LIKE
#undef ARG_POP_LIKE
#undef ARG_LABEL