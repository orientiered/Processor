// #define ARG_NONE
// #define ARG_PUSH_LIKE
// #define ARG_POP_LIKE
// #define ARG_LABEL

#define PUSH(val)           stackPush(&cpu->stk, val)
#define POP()               stackPop(&cpu->stk)
#define PUSH_CALL_STK(val)  stackPush(&cpu->callStk, val)
#define POP_CALL_STK()      stackPop(&cpu->callStk)
#define CHECK_STK_SIZE(minSize)                                                     \
    if (stackGetSize(&cpu->stk) < minSize) {                                        \
        logPrint(L_ZERO, 1, "Stack size < %d at 0x%X: cmd = %d\n", minSize, IP_TO_IDX(IP), *IP);   \
        return false;                                                               \
    }

#define IP                  cpu->ip
#define IP_TO_IDX(pointer)  size_t(pointer - cpu->code)
#define IDX_TO_IP(idx)      (cpu->code + idx)
#define MAKE_JMP            cpu->ip = cpu->code + cpu->ip[CMD_LEN]
#define GET_ARG_PUSH        *getArgs(cpu, 0)
#define GET_ARG_POP         *getArgs(cpu, 1)

#define MATH_TWO(mathOp)            \
    CHECK_STK_SIZE(2);              \
    int a = POP(), b = POP();       \
    PUSH(b mathOp a);               \
    IP += CMD_LEN;

#define MATH_ONE(mathFunc)          \
    CHECK_STK_SIZE(1);              \
    PUSH(mathFunc(POP()));          \
    IP += CMD_LEN;

#define COND_JUMP(condSign)         \
    CHECK_STK_SIZE(2);              \
    int a = POP(), b = POP();       \
    if (a condSign b)               \
        MAKE_JMP;                   \
    else                            \
        IP += CMD_LEN + ARG_LEN;

DEF_CMD_(HLT,   0,  ARG_NONE,
{
    run = false;
})

DEF_CMD_(PUSH,  1,  ARG_PUSH_LIKE,
{
    PUSH(GET_ARG_PUSH);
})
DEF_CMD_(POP,   2,  ARG_POP_LIKE,
{
    GET_ARG_POP = POP();
})

DEF_CMD_(ADD,   3,  ARG_NONE, {MATH_TWO(+)})
DEF_CMD_(SUB,   4,  ARG_NONE, {MATH_TWO(-)})
DEF_CMD_(MUL,   5,  ARG_NONE, {MATH_TWO(*)})
DEF_CMD_(DIV,   6,  ARG_NONE, {MATH_TWO(/)})
DEF_CMD_(SQRT,  7,  ARG_NONE, {MATH_ONE(sqrt)})
DEF_CMD_(SIN,   8,  ARG_NONE, {MATH_ONE(sin)})
DEF_CMD_(COS,   9,  ARG_NONE, {MATH_ONE(cos)})

DEF_CMD_(JMP,   10, ARG_LABEL, {MAKE_JMP;})
DEF_CMD_(JL,    11, ARG_LABEL,
{
    if (rand() % 2 == 0)
        MAKE_JMP;
    else
        IP += CMD_LEN + ARG_LEN;
})
DEF_CMD_(CALL,  12, ARG_LABEL,
{
    PUSH_CALL_STK(IP_TO_IDX(IP + CMD_LEN + ARG_LEN));
    MAKE_JMP;
})
DEF_CMD_(RET,   13, ARG_NONE,
{
    IP = IDX_TO_IP(POP_CALL_STK());
})

DEF_CMD_(JA,    14, ARG_LABEL, {COND_JUMP(> )})
DEF_CMD_(JAE,   15, ARG_LABEL, {COND_JUMP(>=)})
DEF_CMD_(JB,    16, ARG_LABEL, {COND_JUMP(< )})
DEF_CMD_(JBE,   17, ARG_LABEL, {COND_JUMP(<=)})
DEF_CMD_(JE,    18, ARG_LABEL, {COND_JUMP(==)})
DEF_CMD_(JNE,   19, ARG_LABEL, {COND_JUMP(!=)})

DEF_CMD_(SLEEP, 20, ARG_PUSH_LIKE,
{
    usleep(GET_ARG_PUSH * 1000);
})
DEF_CMD_(TIME,  21, ARG_NONE,
{
    gettimeofday(&currentTime, NULL);
    int timeInMs = ((currentTime.tv_sec  - startTime.tv_sec) * 1000000 +
                    (currentTime.tv_usec - startTime.tv_usec)) / 1000;
    PUSH(timeInMs);
    IP += CMD_LEN;
})

DEF_CMD_(IN,    22, ARG_NONE,
{
    int val = 0;
    scanf("%d", &val);
    PUSH(val);
    IP += CMD_LEN;
})
DEF_CMD_(OUT,   23, ARG_NONE,
{
    CHECK_STK_SIZE(1);
    printf("%d\n", POP());
    IP += CMD_LEN;
})
DEF_CMD_(DRAW,  24, ARG_NONE,
{
    drawRAM(cpu, DRAW_HEIGHT, DRAW_WIDTH);
    IP += CMD_LEN;
})
DEF_CMD_(DRAWR, 25, ARG_NONE,
{
    drawRAM(cpu, DRAW_HEIGHT, DRAW_WIDTH);
    IP += CMD_LEN;
    printf("\033[%zuA", DRAW_HEIGHT);
    fflush(stdout);
})

DEF_CMD_(DUMP,  26, ARG_NONE,
{
    enum LogLevel lg = getLogLevel();
    setLogLevel(L_DEBUG);
    cpuDump(cpu);
    IP += CMD_LEN;
    setLogLevel(lg);
})
// DEF_CMD_(CHR,   27, ARG_PUSH_LIKE,
// {
//     printf("%c", GET_ARG_PUSH);
// })

#undef ARG_NONE
#undef ARG_PUSH_LIKE
#undef ARG_POP_LIKE
#undef ARG_LABEL
#undef PUSH
#undef POP
#undef PUSH_CALL_STK
#undef POP_CALL_STK
#undef CHECK_STK_SIZE
#undef IP
#undef IP_TO_IDX
#undef IDX_TO_IP
#undef MAKE_JMP
#undef GET_ARG
