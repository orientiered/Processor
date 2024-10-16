#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "error_debug.h"
#include "logger.h"
#include "argvProcessor.h"
#include "myVector.h"
#include "compiler.h"

char *constructOutName(const char *inName, const char *extensionName);

int main(int argc, const char *argv[]) {
    const char *extensionName = ".lol";
    logOpen();
    logDisableBuffering();
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
    char *outName = isFlagSet("-o")  ?  getFlagValue("-o").string_ :
                                        constructOutName(inputName, extensionName);

    bool compileResult = compile(inputName, outName);


    if (!compileResult)
        logPrint(L_ZERO, 1, "Compilation failed :(\n");
    else
        logPrint(L_ZERO, 1, "Successfully compiled to %s\n", outName);

    if (!isFlagSet("-o"))
            free(outName);

    logClose();

    return (compileResult == true) ? 0 : 1;
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
