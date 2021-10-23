#if !defined(CONFIGURATION_PARSER_H)
#include <vector>

#include "toml.hpp"

using namespace std;

typedef toml::v2::ex::parse_result dict_like_config;


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
        const string packageName;
        const string packageVersion;
        const vector<string> packageAuthors;

        vector<dependency*> dependencies;

        static configuration* FromDictLike(dict_like_config dictLike) {
            // TODO
            return new configuration();
        }
};


bool CheckConfiguration(void*);
void* ParseConfiguration(char*);
void* ParseAndCheckConfiguration(char*);

#define CONFIGURATION_PARSER_H
#endif
