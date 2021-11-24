#include <stdio.h>

#include "dependency_resolver.hpp"
#include "git_lib.cpp"


resolution_result* ResolveGitDependency(application_context& ctx, dependency* dep) {
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
    targetDirectoryNameStream << dep->name << "-" << dep->inputDependency.version;
    std::string targetDirectoryName = targetDirectoryNameStream.str();

    std::ostringstream targetDirectoryPathStream;
    targetDirectoryPathStream << ctx.dependencyPathPrefix << targetDirectoryName;
    std::string targetDirectoryPath = targetDirectoryPathStream.str();

    ctx.applicationLogger->info("Dependency working directory is \"{}\"", targetDirectoryPath);

    resolution_result* resolutionResult = NULL;
    if (utils::DirectoryExists(targetDirectoryPath)) {
        ctx.applicationLogger->info("Dependency \"{}\" already resolved, skipping.", targetDirectoryName);

        return resolutionResult;
    }

    switch (dep->inputDependency.versionType) {
        case (version_type::VERSION_TYPE_DEFAULT):
            {
                resolutionResult = CloneRepo(ctx, dep->inputDependency.source, targetDirectoryPath);
                break;
            }
        default:
            {
                resolutionResult = CloneAndCheckout(ctx, dep->inputDependency.source, targetDirectoryPath, dep->inputDependency.version);
                break;
            }
    }

    if (!resolutionResult || !resolutionResult->resolutionSuccessful) {
        ctx.userLogger->warn("Could not resolve git dependency \"{}\"", dep->name);
    }

    return resolutionResult;
}


void UpdateResolvedDependency(dependency* dep, resolution_result* resolutionResult) {
    lock_dependency* dependencyToUpdate = &dep->lockDependency;

    dependencyToUpdate->localPath = resolutionResult->localPath;
    dependencyToUpdate->resolvedVersion = resolutionResult->version;

    std::ostringstream resolvedSourceStream;
    resolvedSourceStream << SourceTypeToString(dep->inputDependency.sourceType) << '+';
    resolvedSourceStream << dep->inputDependency.source << '#';
    resolvedSourceStream << (resolutionResult->tag.empty() ? resolutionResult->version : resolutionResult->tag);
    dependencyToUpdate->resolvedSource = resolvedSourceStream.str();
}


bool Resolve(application_context& ctx, dependency* dep) {
    bool directoryCreationSuccessful = utils::MakeDirs(ctx, ctx.dependencyPathPrefix,
                                                       utils::directory_creation_mode::IGNORE_IF_EXISTS);

    if (!directoryCreationSuccessful) {
        ctx.applicationLogger->error("Could not process entry \"{}\", failed to create target directory",
                                     dep->name);

        return false;
    }

    resolution_result* resolutionResult = NULL;
    switch (dep->inputDependency.sourceType) {
        case (source_type::SOURCE_TYPE_GIT):
            {
                resolutionResult = ResolveGitDependency(ctx, dep);
                break;
            }
        default:
            {
                ctx.applicationLogger->warn("Unsupported source type {}, ignoring.", dep->inputDependency.sourceType);
                break;
            }
    }


    if (!resolutionResult || !resolutionResult->resolutionSuccessful) {
        return false;
    }

    UpdateResolvedDependency(dep, resolutionResult);
    return true;
}


vector<dependency*>* FilterUnmodified(application_context& ctx, vector<dependency*>& dependencies) {
    // TODO this will probably need to be a bit richer than just a vector of dependencies. Maybe some
    // struct that lets us know why it's considered modified (`NEW`, `UPDATED_VERSION`, etc.), and some extra
    // flags telling us all the actions we need to take.
    vector<dependency*>* modifiedDependencies = new vector<dependency*>();

    return modifiedDependencies;
}

