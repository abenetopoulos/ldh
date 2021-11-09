#if !defined(LOGGER_H)
#include <map>
#include <vector>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace std;

const string APPLICATION_LOGGER_NAME = "APP";
const string USER_LOGGER_NAME = "USR";

typedef spdlog::logger logger_type;
typedef shared_ptr<logger_type> logger_ptr;


class logger_manager{
    public:
        static logger_manager* instance;

        map<string, logger_ptr> componentLoggers;
        vector<spdlog::sink_ptr> sinks;

        logger_manager() {
            this->sinks.push_back(make_shared<spdlog::sinks::stdout_color_sink_mt>()); // stdout sink
        }

        logger_ptr CreateComponentLogger(const string& componentName) {
            logger_ptr componentLogger = make_shared<logger_type>(componentName, begin(this->sinks), end(this->sinks));
            if (componentName == USER_LOGGER_NAME) {
                componentLogger->set_pattern("[%^%l%$]: %v");
            } else {
                componentLogger->set_pattern("[%^%l%$ - %H:%M:%S%z] %@ (tid %t): %v");
            }

            return componentLogger;
        }

        logger_ptr GetLogger(const string& componentName) {
            logger_ptr componentLogger = this->componentLoggers[componentName];

            if (!componentLogger) {
                componentLogger = this->CreateComponentLogger(componentName);
                componentLoggers[componentName] = componentLogger;
            }

            return componentLogger;
        }

        static logger_manager* GetInstance();
};


logger_manager* logger_manager::instance = nullptr;


logger_manager* logger_manager::GetInstance() {
    if (instance == nullptr) {
        instance = new logger_manager();
    }

    return instance;
}

#define LOGGER_H
#endif
