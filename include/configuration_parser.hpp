#if !defined(CONFIGURATION_PARSER_H)
#include <vector>

#include "toml.hpp"

using namespace std;

typedef toml::parse_result dict_like_config;
typedef toml::node_view<toml::node> section;


enum source_type {
    SOURCE_TYPE_GIT = 0,

    SOURCE_TYPE_COUNT,
};


enum version_type {
    VERSION_TYPE_SEMVER = 0,
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


class dependency {
    public:
        string name;

        enum source_type sourceType;
        string source;

        enum version_type versionType;
        string version;
};


class configuration {
    public:
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

        static configuration* FromDictLike(dict_like_config& dictLike) {
            configuration* config = new configuration();
            config->ParsePackageSection(dictLike["package"]);

            return config;
        }
};


bool CheckConfiguration(void*);
void* ParseConfiguration(char*);
void* ParseAndCheckConfiguration(char*);

#define CONFIGURATION_PARSER_H
#endif
