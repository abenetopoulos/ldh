#if !defined(CONFIGURATION_PARSER_H)
#include <cstdint>
#include <vector>

#include "dependency.hpp"
#include "toml.hpp"

using namespace std;

typedef toml::parse_result dict_like_config;
typedef toml::node_view<toml::node> section;


enum class configuration_modes: unsigned {
    CONFIGURATION_MODE_NONE = 0x0,
    CONFIGURATION_MODE_INPUT = 0x1,
    CONFIGURATION_MODE_OUTPUT = 0x2,
};


configuration_modes operator |(configuration_modes lhs, configuration_modes rhs) {
    return static_cast<configuration_modes> (
        static_cast<std::underlying_type<configuration_modes>::type>(lhs) |
        static_cast<std::underlying_type<configuration_modes>::type>(rhs)
    );
}


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


configuration* ParseConfiguration(application_context&, string&, configuration_modes);
bool CheckConfiguration(application_context&, configuration*, configuration_modes);
configuration* ParseAndCheckConfiguration(application_context&, string&, configuration_modes);

bool WriteConfiguration(application_context&, string&, configuration*);

#define CONFIGURATION_PARSER_H
#endif
