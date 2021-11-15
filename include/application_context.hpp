#if !defined(APPLICATION_CONTEXT_H)
#include "command_line.hpp"
#include "logger_manager.hpp"

struct application_context {
    std::string binaryName;

    logger_ptr applicationLogger;
    logger_ptr userLogger;

    execution_arguments* args;

    static const std::string dependencyPathPrefix;
};

// C++ is not fun.
const std::string application_context::dependencyPathPrefix = "target/dependencies/";

#define APPLICATION_CONTEXT_H
#endif
