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


string GenerateLockFilePath(string configurationFilePath) {  // FIXME move somewhere else
    string sourceString = configurationFilePath;
    size_t substringIndex = 0;
    size_t lastIndex = 0;

    // locate index of last backslash
    // NOTE this is platform dependent.
    while ((substringIndex = sourceString.find('/', substringIndex)) != string::npos) {
        lastIndex = substringIndex++;
    }

    return sourceString.replace(sourceString.begin() + lastIndex, sourceString.end(), "/ldh.lock");
}


int main(int argc, char* argv[]) {
    application_context* ctx = new application_context();

    ctx->binaryName = string(argv[0]);
    ctx->applicationLogger = logger_manager::GetInstance()->GetLogger(APPLICATION_LOGGER_NAME);
    ctx->userLogger = logger_manager::GetInstance()->GetLogger(USER_LOGGER_NAME);

    ctx->args = new execution_arguments();
    if (!ParseExecutionArguments(ctx->args, argc, argv)) {
        PrintUsageString(*ctx);
        return 1;
    }

    if (ctx->args->currentMode == mode::MODE_HELP) {
        PrintManPage(*ctx);
        return 0;
    }

    if (ctx->args->lockFilePath.empty()) {
        ctx->args->lockFilePath = GenerateLockFilePath(ctx->args->configurationFilePath);
    }

    configuration_modes mode = configuration_modes::CONFIGURATION_MODE_INPUT | (
            ctx->args->currentMode == mode::MODE_VALIDATE ? configuration_modes::CONFIGURATION_MODE_NONE : configuration_modes::CONFIGURATION_MODE_OUTPUT );

    configuration* config = ParseAndCheckConfiguration(*ctx, ctx->args->configurationFilePath, mode);
    assert(config);

    if (ctx->args->currentMode == mode::MODE_VALIDATE) {
        return 0;
    }

    ctx->applicationLogger->info("will resolve");
    // vector<dependency*>* dependenciesToResolve = FilterUnmodified(*ctx, config->dependencies);
    vector<dependency*>* dependenciesToResolve = &config->dependencies;
    for (dependency* dep: *dependenciesToResolve) {
        if (!Resolve(*ctx, dep)) {
            ctx->applicationLogger->warn("Resolution of \"{}\" failed.", dep->name);
        }
    }

    if (!WriteConfiguration(*ctx, ctx->args->lockFilePath, config)) {
        ctx->applicationLogger->error("Failed while writing lock file to \"{}\"", ctx->args->lockFilePath);

        return 1;
    }

    return 0;
}
