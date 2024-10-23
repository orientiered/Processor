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

const int CTOR_ERROR = 1;

int main(int argc, const char *argv[]) {
    logOpen();
    setLogLevel(L_ZERO);
    registerFlag(TYPE_STRING, "-i", "--input", "input file with code");
    registerFlag(TYPE_BLANK,  "-d", "--debug", "Disables log buffering and sets max logLevel");
    processArgs(argc, argv);

    const char *fileName = "program_code.txt";
    if (isFlagSet("-d")) {
        //logDisableBuffering();
        setLogLevel(L_EXTRA);
    }
    if (isFlagSet("-i"))
        fileName = getFlagValue("-i").string_;

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
