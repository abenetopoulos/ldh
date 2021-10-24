#include <iostream>

#include "configuration_parser.cpp"
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
    logger_ptr logger = logger_manager::GetInstance()->GetLogger(utils::extractFileName(__FILE__));
    PrintUsageString(argv[0]);

    // HACK for now, fix once we have argument parsing.
    assert(argc > 1);
    std::string configurationFilePath(argv[1]);

    auto config = ParseAndCheckConfiguration(configurationFilePath);
    assert(config);

    return 0;
}
