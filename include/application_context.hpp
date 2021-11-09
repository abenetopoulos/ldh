#if !defined(APPLICATION_CONTEXT_H)
#include "logger_manager.hpp"

struct application_context {
    logger_ptr applicationLogger;
    logger_ptr userLogger;
};

#define APPLICATION_CONTEXT_H
#endif
