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
    string path;

    repository(git_repository* r, string p): libRepository(r), path(p) {}
};


struct resolution_result {
    // NOTE this is in this file just for convenience. Ideally, once we add more types of
    // resolvers, this will move to a shared file.
    bool resolutionSuccessful;

    repository* repo;

    string localPath;
    string remote;

    string version;
    string tag;

    resolution_result(bool rs): resolutionSuccessful(rs) { }
};



resolution_result* CloneRepo(application_context&, string, string);
resolution_result* CloneAndCheckout(application_context&, string, string, string);

resolution_result* CreateResolutionResultFromLocalGitRepo(application_context&, string, string, string);

#define GIT_LIB_H
#endif
