#if !defined(UTILS_H)
#include <iostream>

namespace utils {
    enum directory_creation_mode {
        ERROR_IF_EXISTS = 0,
        IGNORE_IF_EXISTS,
    };

    std::string ExtractFileName(const char* filePath) {
        const char* fileName = filePath;
        while (*filePath) {
            if (*filePath++ == '/') {
                fileName = filePath;
            }
        }
        return std::string(fileName);
    }

    bool FileExists(std::string pathStr) {
        std::filesystem::path path = std::filesystem::path(pathStr);

        return std::filesystem::exists(path);
    }

    bool DirectoryExists(std::string pathStr) {
        std::filesystem::path path = std::filesystem::path(pathStr);
        if (!std::filesystem::exists(path)) {
            return false;
        }

        if (!(std::filesystem::is_directory(path) || std::filesystem::is_symlink(path))) {
            return false;
        }

        return true;
    }


    bool RenameNode(application_context& ctx, std::string oldPathToNode, std::string newPathToNode) {
        std::filesystem::path oldPath = std::filesystem::path(oldPathToNode);
        std::filesystem::path newPath = std::filesystem::path(newPathToNode);

        std::error_code nodeRenamingError;
        std::filesystem::rename(oldPath, newPath);

        if (nodeRenamingError) {
            ctx.userLogger->error("Failed while trying to rename node \"{}\" to \"{}\"", oldPathToNode,
                                  newPathToNode);
            ctx.userLogger->error("Reason: {}", nodeRenamingError.message());

            return false;
        }

        return true;
    }

    // FIXME application_context does not really make sense as an argument for a utility method,
    // but for now it will do.
    bool MakeDirs(application_context& ctx, std::string pathStr, directory_creation_mode mode) {
        std::filesystem::path path = std::filesystem::path(pathStr);
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            switch (mode) {
                case (directory_creation_mode::ERROR_IF_EXISTS):
                    {
                        return false;
                    }
                default:
                    {
                        return true;
                    }
            }
        }

        std::error_code directoryCreationError;
        std::filesystem::create_directories(path, directoryCreationError);

        if (directoryCreationError) {
            ctx.userLogger->error("Failed while trying to create directory \"{}\"", pathStr);
            ctx.userLogger->error("Reason: {}", directoryCreationError.message());

            return false;
        }

        return true;
    }

    bool DeleteDirAndContents(application_context& ctx, std::string pathStr) {
        std::filesystem::path path = std::filesystem::path(pathStr);
        if (!(std::filesystem::exists(path) && std::filesystem::is_directory(path))) {
            ctx.applicationLogger->warn("Cannot delete entry \"{}\", path is not directory or does not exist",
                                        pathStr);

            return false;
        }

        std::error_code directoryRemovalError;
        std::filesystem::remove_all(path, directoryRemovalError);

        if (directoryRemovalError) {
            ctx.userLogger->error("Failed while trying to delete directory \"{}\"", pathStr);
            ctx.userLogger->error("Reason: {}", directoryRemovalError.message());

            return false;
        }

        return true;
    }
}

#define UTILS_H
#endif
