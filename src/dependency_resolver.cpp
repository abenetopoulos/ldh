#include <stdio.h>

#include "dependency_resolver.hpp"
#include "git_lib.cpp"


string MatchVersionRange(application_context& ctx, version_t& version, repository* repo) {
    vector<string*>* repositoryTags = GetTagsForRepository(ctx, repo);
    if (repositoryTags->empty()) {
        delete repositoryTags;

        return "";
    }

    string result = "";
    for (string* repositoryTag : *repositoryTags) {
        if (result.empty() && version.VersionsMatch(*repositoryTag)) {
            result = *repositoryTag;
        }

        delete repositoryTag;
    }

    delete repositoryTags;

    return result;
}


resolution_result* ResolveGitDependency(application_context& ctx, dependency* dep) {
    ctx.applicationLogger->info("Proceeding to resolve git dependency \"{}\"", dep->name);

    /* Steps
    *  1) figure out a file path
    *       - as we want to support multiple versions of each package, this is a combination of the
    *       dependency's name & version
    *  2) git clone
    *  3) git checkout the specific version the user requested (if any)
    *  4) potentially rename the target directory
    */

    // path format is $PWD/target/dependencies/name-version
    // unless we're dealing with a semver range, which will be fixed _after_ fetching, in which case we
    // append a `temp` suffix
    string targetDirectoryPrefix = dep->name + "-";
    string targetDirectoryName;

    bool shouldMoveAfterFetching = false;
    if (dep->inputDependency.specifiedVersion.IsExact()) {
        targetDirectoryName = targetDirectoryPrefix + dep->inputDependency.specifiedVersion.exact;
    } else {
        shouldMoveAfterFetching = true;
        targetDirectoryName = targetDirectoryPrefix + "temp";
    }

    std::ostringstream targetDirectoryPathStream;
    targetDirectoryPathStream << ctx.dependencyPathPrefix << targetDirectoryName;
    std::string targetDirectoryPath = targetDirectoryPathStream.str();

    ctx.applicationLogger->info("Dependency working directory is \"{}\"", targetDirectoryPath);

    resolution_result* resolutionResult = NULL;
    version_t requestedVersion = dep->inputDependency.specifiedVersion;

    if (utils::DirectoryExists(targetDirectoryPath)) {
        ctx.applicationLogger->info("Dependency \"{}\" already resolved, skipping.", targetDirectoryName);

        resolutionResult = CreateResolutionResultFromLocalGitRepo(ctx, dep->inputDependency.source,
                                                                  targetDirectoryPath,
                                                                  dep->inputDependency.specifiedVersion);

        if (resolutionResult->tag.empty()) {
            if (requestedVersion.type == version_type::VERSION_TYPE_SEMVER) {
                resolutionResult->tag = MatchVersionRange(ctx, requestedVersion, resolutionResult->repo);
            }
        } else if (resolutionResult->tag == "latest") {
            // FIXME if the user has specified no version, we want to read the fixed (in other words, resolved) version
            // that we checked out earlier, and set that as the tag. tbh, this is kind of hacky, but will work for now.
            // Ideally this should not be here, but it definitely does not belong in `git_lib`, so this seemed like the best
            // place for it for now.
            resolutionResult->tag = resolutionResult->version;
        }

        return resolutionResult;
    }

    switch (requestedVersion.type) {
        case (version_type::VERSION_TYPE_DEFAULT):
            {
                resolutionResult = CloneRepo(ctx, dep->inputDependency.source, targetDirectoryPath);
                break;
            }
        case (version_type::VERSION_TYPE_SEMVER):
            {
                resolutionResult = CloneRepo(ctx, dep->inputDependency.source, targetDirectoryPath);
                if (!resolutionResult) {
                    break;
                }

                string tag = requestedVersion.IsExact() ? requestedVersion.exact : MatchVersionRange(ctx,
                                                                                                     requestedVersion,
                                                                                                     resolutionResult->repo);
                if (tag.empty()) {
                    resolutionResult->resolutionSuccessful = false;
                    break;
                }
                // resolutionResult->tag = tag;

                Checkout(ctx, resolutionResult, tag);
                break;
            }
        default:
            {
                resolutionResult = CloneAndCheckout(ctx, dep->inputDependency.source, targetDirectoryPath,
                                                    requestedVersion.exact);
                break;
            }
    }

    if (!resolutionResult || !resolutionResult->resolutionSuccessful) {
        ctx.userLogger->warn("Could not resolve git dependency \"{}\"", dep->name);
    }

    if (shouldMoveAfterFetching) {
        string finalDirectory = ctx.dependencyPathPrefix + targetDirectoryPrefix + resolutionResult->tag;
        if (utils::DirectoryExists(finalDirectory)) {
            // FIXME this means that we should not have fetched this dependency. We should have created a repo
            // without cloning the target, looked through the tags, and skipped cloning if we found a match.
            utils::DeleteDirAndContents(ctx, targetDirectoryPath);
        } else {
            resolutionResult->resolutionSuccessful = utils::RenameNode(ctx, targetDirectoryPath, finalDirectory);
            resolutionResult->localPath = finalDirectory;
        }
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


bool FetchRemoteDependency(application_context& ctx, dependency* dep) {
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

    delete resolutionResult;

    return true;
}


bool DeleteDependency(application_context& ctx, dependency* dep) {
    string localPath = dep->lockDependency.localPath;

    bool directoryDeletionSuccessful = utils::DeleteDirAndContents(ctx, localPath);
    if (!directoryDeletionSuccessful) {
        ctx.applicationLogger->error("Could not delete dependency at path \"{}\"", localPath);
        return false;
    }

    return true;
}


void ResolveDependencies(application_context& ctx, vector<dependency*> dependencies) {
    for (vector<dependency*>::iterator i = dependencies.begin(); i != dependencies.end(); i++) {
        dependency* dep = *i;
        bool resolutionSuccessful = false;

        if (!dep->inputDependency.HasValue()) {
            resolutionSuccessful = DeleteDependency(ctx, dep);

            if (resolutionSuccessful) {
                // FIXME I think it makes sense to only remove from the lock file if we _actually_ managed
                // to delete the dependency's local contents, but I might be wrong...
                dependencies.erase(i);
                delete dep;
            }
        } else {
            resolutionSuccessful = FetchRemoteDependency(ctx, dep);
        }

        if (!resolutionSuccessful) {
            ctx.applicationLogger->warn("Resolution of \"{}\" failed.", dep->name);
        }
    }
}
