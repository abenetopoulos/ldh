#if !defined(DEPENDENCY_H)

enum class source_type {
    SOURCE_TYPE_GIT = 0,
    SOURCE_TYPE_UNKNOWN,

    SOURCE_TYPE_COUNT,
};


constexpr const char* SourceTypeToString(source_type s) {
    switch (s) {
        case source_type::SOURCE_TYPE_GIT:
            {
                return "git";
            }
        default:
            {
                return "";
            }
    }
}


enum class version_type {
    VERSION_TYPE_SEMVER = 0,
    VERSION_TYPE_DEFAULT,
    VERSION_TYPE_BRANCH,
    VERSION_TYPE_TAG,
    VERSION_TYPE_COMMIT_HASH,

    VERSION_TYPE_COUNT,
};


struct package_information {
    string name;
    string version;
    vector<string> authors;
};


struct input_dependency {
    source_type sourceType;
    string source;

    version_type versionType;
    string version;

    bool has_value() {
        return !(this->source.empty() && this->version.empty());
    }
};


struct lock_dependency {
    string localPath;
    string resolvedSource;
    string resolvedVersion;

    bool has_value() {
        return !(this->localPath.empty() && this->resolvedSource.empty() && this->resolvedVersion.empty());
    }
};


struct dependency {
    string name;

    // previously handled/resolved dependencies should have non-empty values for both input and output dependency
    // dependencies that were newly added should only contain a non-empty input dependency
    // dependencies that should be removed will only have a lock dependency entry
    input_dependency inputDependency;
    lock_dependency lockDependency;
};


#define DEPENDENCY_H
#endif
