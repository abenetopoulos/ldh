#include <stdio.h>

#include "dependency_resolver.hpp"
#include "git_lib.cpp"


bool ResolveGitDependency(application_context& ctx, dependency* dep) {
    ctx.applicationLogger->info("Proceeding to resolve git dependency \"{}\"", dep->name);

    /* Steps
    *  1) figure out a file path
    *       - as we want to support multiple versions of each package, this is a combination of the
    *       dependency's name & version
    *       - in the future, if someone updates the version of this dependency, we need to figure
    *       out a way to i) identify which dependency got updated and ii) remove the old directory
    *  2) git clone
    *  3) git checkout the specific version the user requested (if any)
    *  4) update lock file appropriately [TODO]
    */

    // path format is $PWD/target/dependencies/name-version
    // C++ yuck coming up
    std::ostringstream targetDirectoryNameStream;
    targetDirectoryNameStream << dep->name << "-" << dep->version;
    std::string targetDirectoryName = targetDirectoryNameStream.str();

    std::ostringstream targetDirectoryPathStream;
    targetDirectoryPathStream << ctx.dependencyPathPrefix << targetDirectoryName;
    std::string targetDirectoryPath = targetDirectoryPathStream.str();

    ctx.applicationLogger->debug("Dependency working directory is \"{}\"", targetDirectoryPath);

    if (utils::DirectoryExists(targetDirectoryPath)) {
        ctx.applicationLogger->debug("Dependency \"{}\" already resolved, skipping.", targetDirectoryName);

        return true;
    }

    bool resolutionSuccessful = false;
    switch (dep->versionType) {
        case (version_type::VERSION_TYPE_DEFAULT):
            {
                resolutionSuccessful = CloneRepo(ctx, dep->source, targetDirectoryPath);
                break;
            }
        default:
            {
                resolutionSuccessful = CloneAndCheckout(ctx, dep->source, targetDirectoryPath, dep->version);
                break;
            }
    }

    if (!resolutionSuccessful) {
        ctx.userLogger->warn("Could not resolve git dependency \"{}\"", dep->name);
    }

    return resolutionSuccessful;
}


bool Resolve(application_context& ctx, dependency* dep) {
    bool directoryCreationSuccessful = utils::MakeDirs(ctx, ctx.dependencyPathPrefix,
                                                       utils::directory_creation_mode::IGNORE_IF_EXISTS);

    if (!directoryCreationSuccessful) {
        ctx.applicationLogger->error("Could not process entry \"{}\", failed to create target directory",
                                     dep->name);

        return false;
    }

    switch (dep->sourceType) {
        case (source_type::SOURCE_TYPE_GIT):
            {
                return ResolveGitDependency(ctx, dep);
            }
        default:
            {
                ctx.applicationLogger->warn("Unsupported source type {}, ignoring.", dep->sourceType);
                break;
            }
    }

    return false;
}
