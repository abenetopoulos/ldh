#include "git_lib.hpp"

bool InitializeLibrary(application_context& ctx) {
    if (git_libgit2_init() < 0) {
        ctx.applicationLogger->error("Could not initialize libgit2.");
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        return false;
    }

    return true;
}


bool ShutdownLibrary(application_context& ctx) {
    if (git_libgit2_shutdown() < 0) {
        ctx.applicationLogger->error("Could not shut libgit2 down.");
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        return false;
    }

    return true;
}


string GetHeadId(application_context& ctx, git_repository* repo) {
    git_oid commitObjectId;

    int libError = git_reference_name_to_id(&commitObjectId, repo, "HEAD");
    LIB_ERROR_CHECK(ctx.applicationLogger, "get HEAD id", libError, "");

    return git_oid_tostr_s(&commitObjectId);
}


resolution_result* CloneRepo(application_context& ctx, std::string remoteUrl, std::string path) {
    resolution_result* rs = new resolution_result(false);
    rs->localPath = path;
    rs->remote = remoteUrl;

    if (!InitializeLibrary(ctx)) {
        return rs;
    }

    ctx.applicationLogger->debug("Attempting to clone from remote \"{}\" into \"{}\"", remoteUrl, path);

    git_repository* out = NULL;
    int cloneError = git_clone(&out, remoteUrl.c_str(), path.c_str(), NULL);

    if (cloneError) {
        ctx.userLogger->error("Failed while trying to clone repository \"{}\" into directory \"{}\"",
                              remoteUrl, path);
        ctx.userLogger->error("Reason: {}", git_error_last()->message);

        return rs;
    }

    rs->repo = new repository(out, path);
    rs->version = GetHeadId(ctx, rs->repo->libRepository);

    ShutdownLibrary(ctx);

    rs->resolutionSuccessful = true;
    return rs;
}


repository* GetRepositoryAtPath(std::string path) {
    // TODO
    return NULL;
}


void Checkout(application_context& ctx, resolution_result* rs, std::string tag) {
    // FIXME the following is terrible from a readability perspective...
    ctx.applicationLogger->debug("Attempting to checkout tag \"{}\" for \"{}\"", tag, rs->repo->path);
    // TODO repo consistency checks.

    git_reference *ref;
    git_annotated_commit *checkoutTarget = NULL;
    const char* tagCStr = tag.c_str();

    rs->resolutionSuccessful = false;

    int operationError = git_reference_dwim(&ref, rs->repo->libRepository, tagCStr);
    if (operationError) {
        git_object *obj;
        operationError = git_revparse_single(&obj, rs->repo->libRepository, tagCStr);

        if (operationError) {
            ctx.userLogger->error("Failed to find tag \"{}\" for repository at \"{}\"",
                                  tag, rs->repo->path);

            return;
        }

        git_annotated_commit_lookup(&checkoutTarget, rs->repo->libRepository, git_object_id(obj));
        git_object_free(obj);
    } else {
        git_annotated_commit_from_ref(&checkoutTarget, rs->repo->libRepository, ref);
        git_reference_free(ref);
    }

    git_commit* targetCommit = NULL;
    operationError = git_commit_lookup(&targetCommit, rs->repo->libRepository,
                                       git_annotated_commit_id(checkoutTarget));
    git_commit_free(targetCommit);
    if (operationError) {
        ctx.applicationLogger->error("Lookup for commit that corresponds to tag \"{}\" failed", tag);
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        return;
    }

    operationError = git_checkout_tree(rs->repo->libRepository, (const git_object *) targetCommit, NULL);
    if (operationError) {
        ctx.userLogger->error("Failed while trying to checkout \"{}\" for repository at \"{}\"",
                              tag, rs->repo->path);
        ctx.userLogger->error("Reason: {}", git_error_last()->message);

        return;
    }

    if (!git_annotated_commit_ref(checkoutTarget)) {
        operationError = git_repository_set_head_detached_from_annotated(rs->repo->libRepository, checkoutTarget);
        if (operationError) {
            ctx.applicationLogger->error("Failed while detaching HEAD.");
            ctx.applicationLogger->error("Reason: {}", git_error_last()->message);
        }

        return;
    }

    const char *targetHead;
    if (git_reference_lookup(&ref, rs->repo->libRepository, git_annotated_commit_ref(checkoutTarget))) {
        ctx.applicationLogger->error("Failed while looking up {}.", git_annotated_commit_ref(checkoutTarget));
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        git_reference_free(ref);

        return;
    }

    git_reference* branch = NULL;
    if (git_reference_is_remote(ref)) {
        if (git_branch_create_from_annotated(&branch, rs->repo->libRepository, tagCStr, checkoutTarget, 0)) {
            ctx.applicationLogger->error("Failed while creating branch from reference {}.",
                                         git_annotated_commit_ref(checkoutTarget));
            ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

            git_reference_free(ref);

            return;
        }

        targetHead = git_reference_name(branch);
    } else {
        targetHead = git_annotated_commit_ref(checkoutTarget);
    }

    operationError = git_repository_set_head(rs->repo->libRepository, targetHead);
    git_commit_free(targetCommit);
    git_reference_free(ref);
    git_reference_free(branch);

    if (operationError) {
        ctx.applicationLogger->error("Failed while setting HEAD {}.", targetHead);
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        return;
    }

    rs->tag = tag;
    rs->resolutionSuccessful = true;
}


resolution_result* CloneAndCheckout(application_context& ctx, std::string remoteUrl, std::string path, std::string tag) {
    resolution_result* resolutionResult = NULL;
    if (!InitializeLibrary(ctx)) {
        return resolutionResult;
    }

    resolutionResult = CloneRepo(ctx, remoteUrl, path);
    if (!resolutionResult || !resolutionResult->resolutionSuccessful) {
        ctx.userLogger->error("Could not clone repo \"{}\" to \"{}\", aborting", remoteUrl, path);
        return resolutionResult;
    }

    Checkout(ctx, resolutionResult, tag);
    if (!resolutionResult->resolutionSuccessful) {
        ctx.userLogger->error("Could not checkout tag \"{}\" for \"{}\", aborting", tag, path);
        // cleanup after clone, so that we can retry this cleanly on the next run.
        utils::DeleteDirAndContents(ctx, path);  // FIXME catch error result

        return resolutionResult;
    }

    ShutdownLibrary(ctx);

    return resolutionResult;
}

