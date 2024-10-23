#ifndef CPU_COMMANDS
#define CPU_COMMANDS

typedef struct {
    uint64_t signature;
    uint32_t cmdVersion;
    uint32_t size;
} programHeader_t;

const char * const CPU_SIGNATURE = "ORI-D-_-";
const int CPU_CMD_VERSION = 4;

/// Constants for draw command
const size_t DRAW_HEIGHT = 36;
const size_t DRAW_WIDTH  = 96;
const char WHITE_CHAR = '.';
const char BLACK_CHAR = '#';

//TODO: use these constants in all ip movements
const unsigned CMD_LEN = 1;
const unsigned ARG_LEN = 1;
const unsigned REG_LEN = 1;
/// Command structure
/// x x x | y y y y y
/// M R I | CMD CODE
const uint8_t MASK_IMMEDIATE = 0x20;
const uint8_t MASK_REGISTER  = 0x40;
const uint8_t MASK_MEMORY    = 0x80;
const uint8_t MASK_CMD       = 0x1F;

#define DEF_CMD_(cmdName, cmdIndex, argType, ...) \
    CMD_##cmdName = cmdIndex,
enum CMD_OPS {
    #include "Commands.h"
    CMD_SNTXERR = -1
};
#undef DEF_CMD_

typedef struct {
    const char *name;
    enum CMD_OPS op;
} command_t;

#define DEF_CMD_(cmdName, cmdIndex, argType, ...) \
    {#cmdName, CMD_##cmdName},
const command_t CPU_COMMANDS_ARRAY[] = {
    #include "Commands.h"
    {"syntxerr", CMD_SNTXERR}
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
