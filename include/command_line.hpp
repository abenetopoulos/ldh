#if !defined(COMMAND_LINE_H)
#include "clipp.h"


enum mode {
    MODE_HELP = 0,
    MODE_VALIDATE,
    MODE_UPDATE,
    MODE_INSTALL,

    MODE_CNT,
};

struct execution_arguments {
    clipp::group* cli;

    mode currentMode;

    std::string configurationFilePath;
    std::string lockFilePath;
};


std::string UsageString(execution_arguments* args, const char* binaryName);
bool ParseExecutionArguments(execution_arguments* args, int argc, const char* argv[]);

#define COMMAND_LINE_H
#endif
