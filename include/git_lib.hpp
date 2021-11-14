#if !defined(GIT_LIB_H)

#include "git2.h"

struct repository {
    git_repository* libRepository;
    std::string path;

    repository(git_repository* r, std::string p): libRepository(r), path(p) {}
};

repository* CloneRepo(application_context& ctx, std::string remoteUrl, std::string path);
repository* GetRepositoryAtPath(std::string path);

bool Checkout(application_context& ctx, repository* repo, std::string tag);
bool CloneAndCheckout(application_context& ctx, std::string remoteUrl, std::string path, std::string tag);

#define GIT_LIB_H
#endif
