#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "error_debug.h"
#include "logger.h"
#include "argvProcessor.h"
#include "compiler.h"

int main(int argc, const char *argv[]) {
    const char *extensionName = ".lol";
    logOpen();
    setLogLevel(L_EXTRA);

    registerFlag(TYPE_STRING, "-i", "--input", "Input file name");
    registerFlag(TYPE_STRING, "-o", "--o", "Output file name");
    if (processArgs(argc, argv) != SUCCESS)
        return 1;

    if (!isFlagSet("-i")) {
        logPrint(L_ZERO, 1, "No input file\n");
        logClose();
        return 1;
    }

    const char *inputName = getFlagValue("-i").string_;
    char *outName = NULL;
    if (!isFlagSet("-o")) {
        const char *dotPos = strrchr(inputName, '.');
        if (dotPos == NULL) {
            outName = (char *) calloc(strlen(inputName) + strlen(extensionName) + 1, 1);
            strcpy(outName, inputName);
        } else {
            outName = (char *) calloc(size_t(dotPos - inputName) + strlen(extensionName) + 1, 1);
            strncpy(outName, inputName, size_t(dotPos - inputName));
        }
        strcat(outName, extensionName);
    } else {
        outName = getFlagValue("-o").string_;
    }

    if (!compile(inputName, outName)) {
        if (!isFlagSet("-o"))
            free(outName);
        logPrint(L_ZERO, 1, "Compilation failed :(\n");
        logClose();
        return 1;
    }
    logPrint(L_ZERO, 1, "Successfully compiled to %s\n", outName);
    if (!isFlagSet("-o"))
        free(outName);
    return 0;
}
