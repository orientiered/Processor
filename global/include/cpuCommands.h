#ifndef CPU_COMMANDS
#define CPU_COMMANDS

const char * const CPU_SIGNATURE = "ORI-D";
const int CPU_CMD_VERSION = 1;

const uint8_t MASK_IMMEDIATE = 0x10;
const uint8_t MASK_REGISTER  = 0x20;
enum CMD_OPS {
    CMD_SNTXERR = -1,
    CMD_PUSH = 1,
    CMD_POP,

    CMD_ADD,
    CMD_SUB,
    CMD_MUL,
    CMD_DIV,
    CMD_SQRT,
    CMD_SIN,
    CMD_COS,

    CMD_JMP,
    CMD_JA,
    CMD_IN,
    CMD_OUT,

    CMD_DUMP,
    CMD_HLT,
};

const size_t CPU_REGS_COUNT = 5;
enum CMD_REGS {
    R0X = 0,
    RAX = 1,
    RBX,
    RCX,
    RDX,
    REX,
};

#endif
