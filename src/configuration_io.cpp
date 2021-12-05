#include <filesystem>

#include "utils.hpp"
#include "configuration_io.hpp"
#include "logger_manager.hpp"


namespace fs = std::filesystem;


/***************************************************
 * Input
 ***************************************************/


void configuration::ParsePackageSection(section packageSection) {
    // TODO consider moving inside `package_information`
    this->packageInformation.name = packageSection["name"].value_or(""sv);
    this->packageInformation.version = packageSection["version"].value_or(""sv);

    for (auto&& s : *packageSection["authors"].as_array()) {
        // TODO pointer ownership check
        this->packageInformation.authors.push_back(s.value_or(""));
    }
}


void configuration::ParseDependenciesSection(section dependenciesSection) {
    for (auto&& [dependencyName, dependencyProperties] : *dependenciesSection.as_table()) {
        dependency* entry = new dependency();

        entry->name = dependencyName;
        dependencyProperties.visit([&entry](auto& node) noexcept {
                auto nodeTable = node.as_table();
                input_dependency* dependency = &entry->inputDependency;

                if (!nodeTable->contains("git")) {
                    dependency->sourceType = source_type::SOURCE_TYPE_UNKNOWN;
                    return;
                }

                dependency->sourceType = source_type::SOURCE_TYPE_GIT;
                dependency->source = (*nodeTable)["git"].value_or("");

                version_t* dependencyVersion = &dependency->specifiedVersion;

                string versionKey;
                if (nodeTable->contains("branch")) {
                    dependencyVersion->type = version_type::VERSION_TYPE_BRANCH;
                    versionKey = "branch";
                } else if (nodeTable->contains("tag")) {
                    dependencyVersion->type = version_type::VERSION_TYPE_TAG;
                    versionKey = "tag";
                } else if (nodeTable->contains("commit")) {
                    dependencyVersion->type = version_type::VERSION_TYPE_COMMIT_HASH;
                    versionKey = "commit";
                } else if (nodeTable->contains("version")) {
                    dependencyVersion->type = version_type::VERSION_TYPE_SEMVER;
                    versionKey = "version";
                } else {
                    dependencyVersion->type = version_type::VERSION_TYPE_DEFAULT;
                }

                dependencyVersion->FromString(versionKey.empty() ? "latest" : (*nodeTable)[versionKey].value_or(""));
        });

        this->dependencies.push_back(entry);
    }
}


void ReconcileConfigurationAndLock(application_context& ctx, configuration* configuration, dict_like_config lockFileDict) {
    for (auto&& entry: *(lockFileDict["packages"]).as_array()) {
        lock_dependency lockDependency;

        auto tbl = entry.as_table();
        lockDependency.localPath = (*tbl)["path"].value_or("");
        lockDependency.resolvedSource = (*tbl)["source"].value_or("");
        lockDependency.resolvedVersion = (*tbl)["version"].value_or("");

        string lockDependencyName = (*tbl)["name"].value_or("");

        bool matched = false;
        for (dependency* dep: configuration->dependencies) {
            string fullDependencyName = dep->name + "-" + dep->inputDependency.specifiedVersion.exact;
            if (dep->name != lockDependencyName || lockDependency.localPath.find(fullDependencyName) == string::npos) {
                continue;
            }

            dep->lockDependency = lockDependency;
            matched = true;
        }

        if (matched) {
            continue;
        }

        dependency* dependencyToBeDeleted = new dependency();
        dependencyToBeDeleted->lockDependency = lockDependency;

        ctx.applicationLogger->debug("Will delete {} (present in lock, not in config)", lockDependencyName);
    }
}


configuration* ParseConfiguration(application_context& ctx, string& configurationFilePath, configuration_modes mode) {
    if (!fs::exists(configurationFilePath)) {
        ctx.applicationLogger->error("File not found: \"{}\"", configurationFilePath);
        return nullptr;
    }

    // TODO wrap in internal lib
    dict_like_config configurationDict = toml::parse_file(configurationFilePath);
    configuration* parsedConfiguration = configuration::FromDictLike(configurationDict);

    if ((mode & configuration_modes::CONFIGURATION_MODE_OUTPUT) != configuration_modes::CONFIGURATION_MODE_NONE) {
        string lockFilePath = ctx.GetLockFilePath();
        if (utils::FileExists(lockFilePath)) {
            dict_like_config lockFileDict = toml::parse_file(ctx.GetLockFilePath());
            ReconcileConfigurationAndLock(ctx, parsedConfiguration, lockFileDict);
        }
    }

    return parsedConfiguration;
}


bool CheckConfiguration(application_context& ctx, configuration* config, configuration_modes mode) {
    // TODO:
    //  - rules to check.

    for (dependency* dep : config->dependencies) {
        if (dep->inputDependency.sourceType == source_type::SOURCE_TYPE_UNKNOWN) {
            // TODO do not use the default file ctx.applicationLogger for this.
            ctx.userLogger->error("Unknown dependency type for dependency \"{}\"", dep->name);
            return false;
        }

        // TODO version (type & content) validation
    }

    return true;
}


configuration* ParseAndCheckConfiguration(application_context& ctx, string& configurationFilePath, configuration_modes mode) {
    configuration* config = ParseConfiguration(ctx, configurationFilePath, mode);

    if (!config) {
        ctx.applicationLogger->error("Could not parse config from input \"{}\"", configurationFilePath);
        return nullptr;
    }

    if (!CheckConfiguration(ctx, config, mode)) {
        ctx.applicationLogger->error("Configuration check failed, see logs for more information");
        return nullptr;
    }

    ctx.applicationLogger->info("Config for package \"{}\" OK", config->packageInformation.name);
    return config;
}


/***************************************************
 * Output
 ***************************************************/


toml::table DependencyToTable(dependency* dep) {
    toml::table result;

    result.insert("name", dep->name);
    result.insert("version", dep->lockDependency.resolvedVersion);
    result.insert("source", dep->lockDependency.resolvedSource);
    result.insert("path", dep->lockDependency.localPath);

    return result;
}


bool WriteConfiguration(application_context& ctx, string& outputPath, configuration* config) {
    toml::table outputTable;

    toml::array packagesArray;
    for (dependency* dep : config->dependencies) {
        packagesArray.push_back(DependencyToTable(dep));
    }
    outputTable.insert("packages", packagesArray);

    // NOTE it would be worth it to add a "package" section to the lock file.

    ofstream outputStream;
    outputStream.open(outputPath);
    outputStream << outputTable;
    outputStream.close();

    return true;
}
