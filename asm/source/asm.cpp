#include <stdio.h>

#include "error_debug.h"
#include "logger.h"
#include "argvProcessor.h"
#include "compiler.h"
int main(int argc, const char *argv[]) {
    logOpen();
    registerFlag(TYPE_STRING, "-i", "--input", "Input file name");
    registerFlag(TYPE_STRING, "-o", "--o", "Output file name");
    if (processArgs(argc, argv) != SUCCESS)
        return 1;

    if (!isFlagSet("-i") || !isFlagSet("-o")) {
        logPrint(L_ZERO, 1, "No input or output file\n");
        logClose();
        return 1;
    }

    if (!compile(getFlagValue("-i").string_, getFlagValue("-o").string_)) {
        logPrint(L_ZERO, 1, "Compilation failed :(\n");
        logClose();
        return 1;
    }
    logPrint(L_ZERO, 1, "Successfully compiled to %s\n", getFlagValue("-o").string_);
    return 0;
}
