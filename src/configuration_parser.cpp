#include <filesystem>

#include "utils.hpp"
#include "configuration_parser.hpp"
#include "logger_manager.hpp"


namespace fs = std::filesystem;


configuration* ParseConfiguration(string& configurationFilePath) {
    logger_ptr logger = logger_manager::GetInstance()->GetLogger(utils::ExtractFileName(__FILE__));
    if (!fs::exists(configurationFilePath)) {
        logger->error("File not found: \"{}\"", configurationFilePath);
        return nullptr;
    }

    // TODO wrap in internal lib
    dict_like_config configurationDict = toml::parse_file(configurationFilePath);
    configuration* parsedConfiguration = configuration::FromDictLike(configurationDict);

    return parsedConfiguration;
}


bool CheckConfiguration(configuration* config) {
    // TODO:
    //  - rules to check.
    logger_ptr logger = logger_manager::GetInstance()->GetLogger(utils::ExtractFileName(__FILE__));

    for (dependency* dep : config->dependencies) {
        if (dep->sourceType == source_type::SOURCE_TYPE_UNKNOWN) {
            // TODO do not use the default file logger for this.
            logger->error("Unknown dependency type for dependency \"{}\"", dep->name);
            return false;
        }

        // TODO version (type & content) validation
    }

    return true;
}


void* ParseAndCheckConfiguration(string& configurationFilePath) {
    logger_ptr logger = logger_manager::GetInstance()->GetLogger(utils::ExtractFileName(__FILE__));
    auto config = ParseConfiguration(configurationFilePath);

    if (!config) {
        logger->error("Could not parse config from input \"{}\"", configurationFilePath);
        return nullptr;
    }

    if (!CheckConfiguration(config)) {
        logger->error("Configuration check failed, see logs for more information");
        return nullptr;
    }

    logger->info("Config for package \"{}\" OK", config->packageInformation.name);
    return config;
}
