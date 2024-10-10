#include <stdio.h>

#include "error_debug.h"
#include "logger.h"
#include "argvProcessor.h"
#include "compiler.h"
int main(int argc, const char *argv[]) {
    logOpen();
    registerFlag(TYPE_STRING, "-i", "--input", "Input file name");
    registerFlag(TYPE_STRING, "-o", "--o", "Output file name");
    processArgs(argc, argv);

    compile(getFlagValue("-i").string_, getFlagValue("-o").string_);
    return 0;
}
