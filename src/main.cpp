#include <iostream>

#include "application_context.hpp"
#include "configuration_parser.cpp"
#include "dependency_resolver.cpp"
#include "logger_manager.hpp"
#include "spdlog/spdlog.h"


void PrintUsageString(const char* binName) {
    // TODO
    std::cout << binName << " <verb> [options]" << std::endl
        << "Verbs:" << std::endl
        << "\t" << "check" << "\t" << "Check configuration file." << std::endl
        << "\t" << "help" << "\t" << "Show this usage string." << std::endl;
}

int main(int argc, const char* argv[]) {
    application_context* ctx = new application_context();

    ctx->applicationLogger = logger_manager::GetInstance()->GetLogger(APPLICATION_LOGGER_NAME);
    ctx->userLogger = logger_manager::GetInstance()->GetLogger(USER_LOGGER_NAME);

    PrintUsageString(argv[0]);

    // HACK for now, fix once we have argument parsing.
    assert(argc > 1);
    std::string configurationFilePath(argv[1]);

    configuration* config = ParseAndCheckConfiguration(*ctx, configurationFilePath);
    assert(config);

    for (dependency* dep: config->dependencies) {
        if (!Resolve(*ctx, dep)) {
            ctx->applicationLogger->warn("Resolution of {} failed.", dep->name);
        }
    }

    return 0;
}
