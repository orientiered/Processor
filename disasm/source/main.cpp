#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_debug.h"
#include "logger.h"
#include "argvProcessor.h"
#include "disasm.h"

char *constructOutName(const char *inputName, const char* extensionName);

int main(int argc, const char *argv[]) {
    const char *extensionName = "_D.asm";
    logOpen();
    setLogLevel(L_EXTRA);

    registerFlag(TYPE_STRING, "-i", "--input",   "Input file name");
    registerFlag(TYPE_STRING, "-o", "--output",  "Output file name(optional)");
    enableHelpFlag("--Disassembler for spu--\nUsage: ./dsm.out [args]\n");

    if (processArgs(argc, argv) != ARGV_SUCCESS)
        return 1;

    if (!isFlagSet("-i")) {
        logPrint(L_ZERO, 1, "No input file\n");
        logClose();
        return 1;
    }

    const char *inputName = getFlagValue("-i").string_;
    char *outName = isFlagSet("-o")  ?  getFlagValue("-o").string_ :
                                        constructOutName(inputName, extensionName);

    if (!disasm(inputName, outName))
        logPrint(L_ZERO, 1, "Disassembly failed\n");
    else
        logPrint(L_ZERO, 1, "Successfully disassembled to '%s'\n", outName);

    if (!isFlagSet("-o"))
        free(outName);

    return 0;
}

char *constructOutName(const char *inputName, const char* extensionName) {
    char *outName = NULL;
    const char *dotPos = strrchr(inputName, '.');
    if (dotPos == NULL) {
        outName = (char *) calloc(strlen(inputName) + strlen(extensionName) + 1, 1);
        strcpy(outName, inputName);
    } else {
        outName = (char *) calloc(size_t(dotPos - inputName) + strlen(extensionName) + 1, 1);
        strncpy(outName, inputName, size_t(dotPos - inputName));
    }
    strcat(outName, extensionName);
    return outName;

}


