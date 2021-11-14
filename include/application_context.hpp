#if !defined(APPLICATION_CONTEXT_H)
#include "logger_manager.hpp"

struct application_context {
    logger_ptr applicationLogger;
    logger_ptr userLogger;

    static const std::string dependencyPathPrefix;
};

// C++ is not fun.
const std::string application_context::dependencyPathPrefix = "target/dependencies/";

#define APPLICATION_CONTEXT_H
#endif
