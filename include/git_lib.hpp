#if !defined(GIT_LIB_H)

#include "git2.h"

// NOTE `EMPTY()` macro used for cases where we want to return from a `void` method.
#define EMPTY()
#define GIT_LIB_ERROR_CHECK(lgr, op, rc, rv) \
    if ((rc)) { \
        (lgr)->error("libgit operation {} failed", op); \
        (lgr)->error("Reason: {}", git_error_last()->message); \
        return rv; \
    }


struct repository {
    git_repository* libRepository;
    std::string path;

    repository(git_repository* r, std::string p): libRepository(r), path(p) {}
};


struct resolution_result {
    bool resolutionSuccessful;

    repository* repo;

    std::string localPath;
    std::string remote;

    std::string version;
    std::string tag;

    resolution_result(bool rs): resolutionSuccessful(rs) { }
};


resolution_result* CloneRepo(application_context& ctx, std::string remoteUrl, std::string path);
resolution_result* CloneAndCheckout(application_context& ctx, std::string remoteUrl, std::string path, std::string tag);

#define GIT_LIB_H
#endif
