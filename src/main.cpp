#include <iostream>

#include "application_context.hpp"
#include "command_line.cpp"
#include "configuration_io.cpp"
#include "dependency_resolver.cpp"
#include "logger_manager.hpp"
#include "spdlog/spdlog.h"


void PrintUsageString(application_context& ctx) {
    std::cout << UsageString(ctx.args, ctx.binaryName.c_str()) << "\n";
}


void PrintManPage(application_context& ctx) {
    std::cout << ManPage(ctx.args, ctx.binaryName.c_str());
}


int main(int argc, char* argv[]) {
    application_context* ctx = new application_context();

    ctx->binaryName = std::string(argv[0]);
    ctx->applicationLogger = logger_manager::GetInstance()->GetLogger(APPLICATION_LOGGER_NAME);
    ctx->userLogger = logger_manager::GetInstance()->GetLogger(USER_LOGGER_NAME);

    ctx->args = new execution_arguments();
    if (!ParseExecutionArguments(ctx->args, argc, argv)) {
        PrintUsageString(*ctx);
        return 1;
    }

    if (ctx->args->currentMode == mode::MODE_HELP) {
        PrintManPage(*ctx);
        return 1;
    }

    configuration* config = ParseAndCheckConfiguration(*ctx, ctx->args->configurationFilePath);
    assert(config);

    if (ctx->args->currentMode == mode::MODE_VALIDATE) {
        return 0;
    }

    for (dependency* dep: config->dependencies) {
        if (!Resolve(*ctx, dep)) {
            ctx->applicationLogger->warn("Resolution of \"{}\" failed.", dep->name);
        }
    }

    return 0;
}
