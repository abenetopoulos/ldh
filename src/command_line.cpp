#include "command_line.hpp"


std::string UsageString(execution_arguments* args, const char* binaryName) {
    return clipp::usage_lines(*args->cli, binaryName).str();
}


std::string ManPage(execution_arguments* args, const char* binaryName) {
    std::ostringstream manPageStream;
    manPageStream << clipp::make_man_page(*args->cli, binaryName);

    return manPageStream.str();
}


bool ParseExecutionArguments(execution_arguments* args, int argc, char* argv[]) {
    // default to `help`
    args->currentMode = mode::MODE_HELP;

    clipp::parameter helpMode = clipp::command("help").set(args->currentMode, mode::MODE_HELP);
    clipp::parameter filename = clipp::value("fname",
                                         args->configurationFilePath).if_missing([]{
                                             std::cout << "A configuration file is required\n";
                                         } );

    clipp::group validateMode = (
            clipp::command("validate").set(args->currentMode, mode::MODE_VALIDATE),
            filename );

    clipp::group updateMode = (
            clipp::command("update").set(args->currentMode, mode::MODE_UPDATE),
            filename );

    args->cli = new clipp::group();
    *args->cli = validateMode | updateMode | helpMode;

    return clipp::parse(argc, argv, *args->cli) ? true : false;
}
