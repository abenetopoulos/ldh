#include <filesystem>

#include "utils.hpp"
#include "configuration_parser.hpp"
#include "logger_manager.hpp"


namespace fs = std::filesystem;


configuration* ParseConfiguration(application_context& ctx, string& configurationFilePath) {
    if (!fs::exists(configurationFilePath)) {
        ctx.applicationLogger->error("File not found: \"{}\"", configurationFilePath);
        return nullptr;
    }

    // TODO wrap in internal lib
    dict_like_config configurationDict = toml::parse_file(configurationFilePath);
    configuration* parsedConfiguration = configuration::FromDictLike(configurationDict);

    return parsedConfiguration;
}


bool CheckConfiguration(application_context& ctx, configuration* config) {
    // TODO:
    //  - rules to check.

    for (dependency* dep : config->dependencies) {
        if (dep->sourceType == source_type::SOURCE_TYPE_UNKNOWN) {
            // TODO do not use the default file ctx.applicationLogger for this.
            ctx.userLogger->error("Unknown dependency type for dependency \"{}\"", dep->name);
            return false;
        }

        // TODO version (type & content) validation
    }

    return true;
}


configuration* ParseAndCheckConfiguration(application_context& ctx, string& configurationFilePath) {
    auto config = ParseConfiguration(ctx, configurationFilePath);

    if (!config) {
        ctx.applicationLogger->error("Could not parse config from input \"{}\"", configurationFilePath);
        return nullptr;
    }

    if (!CheckConfiguration(ctx, config)) {
        ctx.applicationLogger->error("Configuration check failed, see logs for more information");
        return nullptr;
    }

    ctx.applicationLogger->info("Config for package \"{}\" OK", config->packageInformation.name);
    return config;
}
