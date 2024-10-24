#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "error_debug.h"
#include "logger.h"
#include "argvProcessor.h"
#include "cStack.h"
#include "cpuCommands.h"
#include "processor.h"

const int CTOR_ERROR      = 1;
const int HELP_MSG_EXIT   = 2;
const int ARGV_ERROR_EXIT = 3;

int main(int argc, const char *argv[]) {
    logOpen();
    setLogLevel(L_ZERO);
    registerFlag(TYPE_STRING, "-i", "--input", "input file with code");
    registerFlag(TYPE_BLANK,  "-d", "--debug", "Disables log buffering and sets max logLevel");
    enableHelpFlag("--Software processing unit--\nUsage: ./spu.out [arguments]\n");
    switch(processArgs(argc, argv)) {
    case ARGV_HELP_MSG:     return HELP_MSG_EXIT;
    case ARGV_ERROR:        return ARGV_ERROR_EXIT;
    default:                break;
    }

    const char *fileName = "program.lol";
    if (isFlagSet("-d")) {
        //logDisableBuffering();
        setLogLevel(L_EXTRA);
    }
    if (isFlagSet("-i"))
        fileName = getFlagValue("-i").string_;
    else
        logPrint(L_ZERO, 1, "Using default program name: %s\n", fileName);

    srand(time(NULL));
    cpu_t cpu = {0};
    if (!cpuCtor(&cpu, fileName)) {
        logPrint(L_ZERO, 1, "Terminating\n");
        return CTOR_ERROR;
    }

    if (!cpuRun(&cpu)) {
        setLogLevel(L_DEBUG);
        cpuDump(&cpu);
    }
    cpuDtor(&cpu);
    logClose();
    return 0;
}
