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
    GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "get HEAD id", libError, "");

    return git_oid_tostr_s(&commitObjectId);
}


resolution_result* CloneRepo(application_context& ctx, std::string remoteUrl, std::string path) {
    resolution_result* rs = new resolution_result(false);
    rs->localPath = path;
    rs->remote = remoteUrl;

    if (!InitializeLibrary(ctx)) {
        return rs;
    }

    ctx.applicationLogger->info("Attempting to clone from remote \"{}\" into \"{}\"", remoteUrl, path);

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


git_annotated_commit* ResolveRemoteReference(application_context& ctx, git_repository* repo, const char* targetReference) {
    git_strarray remotes = { NULL, 0 };
    git_annotated_commit *result = NULL;

    int libError = git_remote_list(&remotes, repo);
    GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "list remotes", libError, NULL);

    git_reference *remoteReference = NULL;
    for (unsigned int i = 0; (i < remotes.count && !remoteReference); i++) {
        char *refname = NULL;

        // figure out the "appropriate" length for `refname`
        unsigned int reflen = snprintf(refname, 0, "refs/remotes/%s/%s", remotes.strings[i], targetReference);
        if (reflen < 0 || !(refname = (char *) malloc(reflen + 1))) {
            break;
        }
        snprintf(refname, reflen + 1, "refs/remotes/%s/%s", remotes.strings[i], targetReference);

        libError = git_reference_lookup(&remoteReference, repo, refname);
        free(refname);
        GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "reference lookup", libError, NULL);
    }
    git_strarray_dispose(&remotes);

    if (!remoteReference) {
        return NULL;
    }

    libError = git_annotated_commit_from_ref(&result, repo, remoteReference);
    git_reference_free(remoteReference);
    GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "annotated commit from ref", libError, NULL);

    return result;
}


git_annotated_commit* ResolveLocalReference(application_context& ctx, git_repository* repo, const char* targetReference) {
    git_reference *ref;
    git_annotated_commit *result = NULL;

    int operationError = git_reference_dwim(&ref, repo, targetReference);
    if (operationError) {
        git_object *obj;

        operationError = git_revparse_single(&obj, repo, targetReference);
        GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "find tag", operationError, NULL);

        git_annotated_commit_lookup(&result, repo, git_object_id(obj));
        git_object_free(obj);
    } else {
        git_annotated_commit_from_ref(&result, repo, ref);
        git_reference_free(ref);
    }

    return result;
}


git_annotated_commit* ResolveReference(application_context& ctx, git_repository* repo, const char* targetReference) {
    git_annotated_commit* result = ResolveLocalReference(ctx, repo, targetReference);
    return result ? result : ResolveRemoteReference(ctx, repo, targetReference);
}


void Checkout(application_context& ctx, resolution_result* rs, std::string tag) {
    // TODO repo consistency checks.
    ctx.applicationLogger->info("Attempting to checkout tag \"{}\" for \"{}\"", tag, rs->repo->path);

    rs->resolutionSuccessful = false;

    const char* tagCStr = tag.c_str();
    git_annotated_commit *checkoutTarget = ResolveReference(ctx, rs->repo->libRepository, tagCStr);
    if (!checkoutTarget) {
        return;
    }

    git_commit* targetCommit = NULL;
    int operationError = git_commit_lookup(&targetCommit, rs->repo->libRepository,
                                       git_annotated_commit_id(checkoutTarget));
    git_commit_free(targetCommit);
    GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "lookup for tag", operationError, EMPTY());

    operationError = git_checkout_tree(rs->repo->libRepository, (const git_object *) targetCommit, NULL);
    GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "checkout", operationError, EMPTY());

    if (!git_annotated_commit_ref(checkoutTarget)) {
        operationError = git_repository_set_head_detached_from_annotated(rs->repo->libRepository, checkoutTarget);
        GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "detaching HEAD", operationError, EMPTY());
    }

    const char *targetHead;
    git_reference *ref;
    operationError = git_reference_lookup(&ref, rs->repo->libRepository, git_annotated_commit_ref(checkoutTarget));
    if (operationError) {
        ctx.applicationLogger->error("Failed while looking up {}.", git_annotated_commit_ref(checkoutTarget));
        ctx.applicationLogger->error("Reason: {}", git_error_last()->message);

        git_reference_free(ref);

        return;
    }

    git_reference* branch = NULL;
    if (git_reference_is_remote(ref)) {
        operationError = git_branch_create_from_annotated(&branch, rs->repo->libRepository, tagCStr, checkoutTarget, 0);
        if (operationError) {
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

    GIT_LIB_ERROR_CHECK(ctx.applicationLogger, "setting HEAD", operationError, EMPTY());

    rs->tag = tag;
    rs->version = GetHeadId(ctx, rs->repo->libRepository);
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

