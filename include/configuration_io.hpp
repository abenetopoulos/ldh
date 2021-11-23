#if !defined(CONFIGURATION_PARSER_H)
#include <vector>

#include "dependency.hpp"
#include "toml.hpp"

using namespace std;

typedef toml::parse_result dict_like_config;
typedef toml::node_view<toml::node> section;


struct configuration {
    package_information packageInformation;
    vector<dependency*> dependencies;

    void ParsePackageSection(section packageSection);
    void ParseDependenciesSection(section dependenciesSection);

    static configuration* FromDictLike(dict_like_config& dictLike) {
        configuration* config = new configuration();
        config->ParsePackageSection(dictLike["package"]);
        config->ParseDependenciesSection(dictLike["dependencies"]);

        return config;
    }
};


bool CheckConfiguration(application_context&, void*);
configuration* ParseConfiguration(application_context&, string&);
configuration* ParseAndCheckConfiguration(application_context&, string&);

#define CONFIGURATION_PARSER_H
#endif
