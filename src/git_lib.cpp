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


repository* CloneRepo(application_context& ctx, std::string remoteUrl, std::string path) {
    if (!InitializeLibrary(ctx)) {
        return NULL;
    }

    ctx.applicationLogger->debug("Attempting to clone from remote \"{}\" into \"{}\"", remoteUrl, path);

    git_repository* out = NULL;
    int cloneError = git_clone(&out, remoteUrl.c_str(), path.c_str(), NULL);

    if (cloneError) {
        ctx.userLogger->error("Failed while trying to clone repository \"{}\" into directory \"{}\"",
                              remoteUrl, path);
        ctx.userLogger->error("Reason: {}", git_error_last()->message);

        return NULL;
    }

    repository *result = new repository(out, path);

    ShutdownLibrary(ctx);

    return result;
}


repository* GetRepositoryAtPath(std::string path) {
    // TODO
    return NULL;
}


bool Checkout(application_context& ctx, repository* repo, std::string tag) {
    // FIXME the following is terrible from a readability perspective...
    ctx.applicationLogger->debug("Attempting to checkout tag \"{}\" for \"{}\"", tag, repo->path);
    // TODO repo consistency checks.

    git_reference *ref;
    git_annotated_commit *checkoutTarget = NULL;
    const char* tagCStr = tag.c_str();

    int operationError = git_reference_dwim(&ref, repo->libRepository, tagCStr);
    if (operationError) {
        git_object *obj;
        operationError = git_revparse_single(&obj, repo->libRepository, tagCStr);

        if (operationError) {
            ctx.userLogger->error("Failed to find tag \"{}\" for repository at \"{}\"",
                                  tag, repo->path);

            return false;
        }

        git_annotated_commit_lookup(&checkoutTarget, repo->libRepository, git_object_id(obj));
        git_object_free(obj);
    } else {
        git_annotated_commit_from_ref(&checkoutTarget, repo->libRepository, ref);
        git_reference_free(ref);
    }

    git_commit* targetCommit = NULL;
    operationError = git_commit_lookup(&targetCommit, repo->libRepository,
                                       git_annotated_commit_id(checkoutTarget));
    git_commit_free(targetCommit);
    if (operationError) {
        ctx.applicationLogger->error("Lookup for commit that corresponds to tag \"{}\" failed", tag);
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        return false;
    }

    operationError = git_checkout_tree(repo->libRepository, (const git_object *) targetCommit, NULL);
    if (operationError) {
        ctx.userLogger->error("Failed while trying to checkout \"{}\" for repository at \"{}\"",
                              tag, repo->path);
        ctx.userLogger->error("Reason: {}", git_error_last()->message);

        return false;
    }

    if (!git_annotated_commit_ref(checkoutTarget)) {
        operationError = git_repository_set_head_detached_from_annotated(repo->libRepository, checkoutTarget);
        if (operationError) {
            ctx.applicationLogger->error("Failed while detaching HEAD.");
            ctx.applicationLogger->error("Reason: {}", git_error_last()->message);
        }

        return false;
    }

    const char *targetHead;
    if (git_reference_lookup(&ref, repo->libRepository, git_annotated_commit_ref(checkoutTarget))) {
        ctx.applicationLogger->error("Failed while looking up {}.", git_annotated_commit_ref(checkoutTarget));
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        git_reference_free(ref);

        return false;
    }

    git_reference* branch = NULL;
    if (git_reference_is_remote(ref)) {
        if (git_branch_create_from_annotated(&branch, repo->libRepository, tagCStr, checkoutTarget, 0)) {
            ctx.applicationLogger->error("Failed while creating branch from reference {}.",
                                         git_annotated_commit_ref(checkoutTarget));
            ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

            git_reference_free(ref);

            return false;
        }

        targetHead = git_reference_name(branch);
    } else {
        targetHead = git_annotated_commit_ref(checkoutTarget);
    }

    operationError = git_repository_set_head(repo->libRepository, targetHead);
    git_commit_free(targetCommit);
    git_reference_free(ref);
    git_reference_free(branch);

    if (operationError) {
        ctx.applicationLogger->error("Failed while setting HEAD {}.", targetHead);
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        return false;
    }

    return true;
}


bool CloneAndCheckout(application_context& ctx, std::string remoteUrl, std::string path, std::string tag) {
    if (!InitializeLibrary(ctx)) {
        return false;
    }

    repository* repo = CloneRepo(ctx, remoteUrl, path);
    if (!repo) {
        ctx.userLogger->error("Could not clone repo \"{}\" to \"{}\", aborting", remoteUrl, path);
        return false;
    }

    if (!Checkout(ctx, repo, tag)) {
        ctx.userLogger->error("Could not checkout tag {} for {}, aborting", tag, path);
        // cleanup after clone, so that we can retry this cleanly on the next run.
        utils::DeleteDirAndContents(ctx, path);  // FIXME catch error result

        return false;
    }

    ShutdownLibrary(ctx);

    return true;
}

