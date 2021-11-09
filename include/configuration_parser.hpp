#if !defined(CONFIGURATION_PARSER_H)
#include <vector>

#include "toml.hpp"

using namespace std;

typedef toml::parse_result dict_like_config;
typedef toml::node_view<toml::node> section;


enum source_type {
    SOURCE_TYPE_GIT = 0,
    SOURCE_TYPE_UNKNOWN,

    SOURCE_TYPE_COUNT,
};


enum version_type {
    VERSION_TYPE_SEMVER = 0,
    VERSION_TYPE_DEFAULT,
    VERSION_TYPE_BRANCH,
    VERSION_TYPE_TAG,
    VERSION_TYPE_COMMIT_HASH,

    VERSION_TYPE_COUNT,

};


struct package_information {
    string name;
    string version;
    vector<string> authors;
};


struct dependency {
    string name;

    enum source_type sourceType;
    string source;

    enum version_type versionType;
    string version;
};


struct configuration {
    package_information packageInformation;
    vector<dependency*> dependencies;

    void ParsePackageSection(section packageSection) {
        // TODO consider moving inside `package_information`
        this->packageInformation.name = packageSection["name"].value_or(""sv);
        this->packageInformation.version = packageSection["version"].value_or(""sv);

        for (auto&& s : *packageSection["authors"].as_array()) {
            // TODO pointer ownership check
            this->packageInformation.authors.push_back(s.value_or(""));
        }
    }

    void ParseDependenciesSection(section dependenciesSection) {
        for (auto&& [dependencyName, dependencyProperties] : *dependenciesSection.as_table()) {
            dependency* entry = new dependency();

            entry->name = dependencyName;
            dependencyProperties.visit([&entry](auto& node) noexcept {
                    auto nodeTable = node.as_table();

                    if (!nodeTable->contains("git")) {
                        entry->sourceType = source_type::SOURCE_TYPE_UNKNOWN;
                        return;
                    }

                    entry->sourceType = source_type::SOURCE_TYPE_GIT;
                    entry->source = (*nodeTable)["git"].value_or("");

                    string versionKey;
                    if (nodeTable->contains("branch")) {
                        entry->versionType = version_type::VERSION_TYPE_BRANCH;
                        versionKey = "branch";
                    } else if (nodeTable->contains("tag")) {
                        entry->versionType = version_type::VERSION_TYPE_TAG;
                        versionKey = "tag";
                    } else if (nodeTable->contains("commit")) {
                        entry->versionType = version_type::VERSION_TYPE_COMMIT_HASH;
                        versionKey = "commit";
                    } else {
                        entry->versionType = version_type::VERSION_TYPE_DEFAULT;
                    }

                    entry->version = versionKey.empty() ? "latest" : (*nodeTable)[versionKey].value_or("");
            });

            this->dependencies.push_back(entry);
        }
    }

    static configuration* FromDictLike(dict_like_config& dictLike) {
        configuration* config = new configuration();
        config->ParsePackageSection(dictLike["package"]);
        config->ParseDependenciesSection(dictLike["dependencies"]);

        return config;
    }
};


bool CheckConfiguration(application_context&, void*);
void* ParseConfiguration(application_context&, char*);
void* ParseAndCheckConfiguration(application_context&, char*);

#define CONFIGURATION_PARSER_H
#endif
