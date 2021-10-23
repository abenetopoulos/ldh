#if !defined(UTILS_H)
#include <iostream>

namespace utils {
    std::string extractFileName(const char* filePath) {
        const char* fileName = filePath;
        while (*filePath) {
            if (*filePath++ == '/') {
                fileName = filePath;
            }
        }
        return std::string(fileName);
    }
}

#define UTILS_H
#endif
