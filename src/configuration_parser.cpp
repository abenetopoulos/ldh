#include <filesystem>

#include "utils.hpp"
#include "configuration_parser.hpp"
#include "logger_manager.hpp"


namespace fs = std::filesystem;

class configuration {
    // TODO
};

void* ParseConfiguration(string& configurationFilePath) {
    logger_ptr logger = logger_manager::GetInstance()->GetLogger(utils::extractFileName(__FILE__));
    if (!fs::exists(configurationFilePath)) {
        logger->error("File not found: \"{}\"", configurationFilePath);
        return nullptr;
    }
    return nullptr;
}


bool CheckConfiguration(void* config) {
    // TODO:
    //  - specify input type (parsed TOML, maybe converted to internal representation?
    //  - rules to check.
    return true;
}


void* ParseAndCheckConfiguration(string& configurationFilePath) {
    logger_ptr logger = logger_manager::GetInstance()->GetLogger(utils::extractFileName(__FILE__));
    auto config = ParseConfiguration(configurationFilePath);

    if (!config) {
        logger->error("Could not parse config from input \"{}\"", configurationFilePath);
        return nullptr;
    }

    if (!CheckConfiguration(config)) {
        logger->error("Configuration check failed, see logs for more information");
        return nullptr;
    }

    return config;
}
