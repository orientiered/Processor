#ifndef CPU_COMMANDS
#define CPU_COMMANDS

const char * const CPU_SIGNATURE = "ORI-D";
const int CPU_CMD_VERSION = 2;

//TODO: use these constants in all ip movements
const unsigned CMD_LEN = 1;
const unsigned ARG_LEN = 1;

/// Command structure
/// x x x | y y y y y
/// M R I | CMD CODE
const uint8_t MASK_IMMEDIATE = 0x20;
const uint8_t MASK_REGISTER  = 0x40;
const uint8_t MASK_MEMORY    = 0x80;
const uint8_t MASK_CMD       = 0x1F;

enum CMD_OPS {
    CMD_SNTXERR = -1,
    CMD_HLT     =  0,

    CMD_PUSH    =  1,
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
    CMD_JAE,
    CMD_JB,
    CMD_JBE,
    CMD_JE,
    CMD_JNE,
    CMD_JL,     ///< Jump on luck (random)

    CMD_CALL,
    CMD_RET,

    CMD_IN,
    CMD_OUT,

    CMD_DUMP,
};

typedef struct {
    const char *name;
    enum CMD_OPS op;
} command_t;
const command_t CPU_COMMANDS_ARRAY[] = {
    {"hlt"  ,   CMD_HLT     },

    {"push" ,   CMD_PUSH    },
    {"pop"  ,   CMD_POP     },

    {"add"  ,   CMD_ADD     },
    {"sub"  ,   CMD_SUB     },
    {"mul"  ,   CMD_MUL     },
    {"div"  ,   CMD_DIV     },
    {"sqrt" ,   CMD_SQRT    },

    {"jmp"  ,   CMD_JMP     },
    {"ja"   ,   CMD_JA      },
    {"jae"  ,   CMD_JAE     },
    {"jb"   ,   CMD_JB      },
    {"jbe"  ,   CMD_JBE     },
    {"je"   ,   CMD_JE      },
    {"jne"  ,   CMD_JNE     },
    {"jl"   ,   CMD_JL      },

    {"call" ,   CMD_CALL    },
    {"ret"  ,   CMD_RET     },

    {"in"   ,   CMD_IN      },
    {"out"  ,   CMD_OUT     },

    {"dump" ,   CMD_DUMP    }
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
