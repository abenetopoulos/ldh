#if !defined(DEPENDENCY_H)

#include "semver.hpp"

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


struct version_t {
    version_type type;
    string exact;

    string versionRange;

    version_t() {
        this->type = version_type::VERSION_TYPE_DEFAULT;
        this->exact = "";
        this->versionRange = "";
    }

    void FromString(string v) {
        if (this->IsExact() || this->IsExactSemVer(v)) {
            this->exact = v;

            return;
        }

        this->versionRange = v;
    }

    bool VersionsMatch(string versionToMatch) {
        if (!this->exact.empty()) {
            return this->exact == versionToMatch;
        }

        semver::version semverVersion{versionToMatch};

        // N.b. ideally we would like to hold a reference to this range instead of converting
        // from a string to a range every time we want to make a comparison, but the library we're
        // relying upon for semver is _terrible_, and keeping around a pointer to the range would cause
        // comparisons with it to enter an infinite loop.
        // Keeping the value around (as opposed to a pointer) is a non-starter, because the `range` type
        // does not define a default constructor.
        // Hence, this mess.
        auto range = semver::range::detail::range(this->versionRange);
        return range.satisfies(semverVersion, true);
    }

    bool IsExact() {
        return this->type != version_type::VERSION_TYPE_SEMVER;
    }

    bool IsExactSemVer(string v) {
        return semver::valid(v);
    }

    bool Empty() {
        return exact.empty();
    }
};


struct input_dependency {
    source_type sourceType;
    string source;

    version_t specifiedVersion;

    bool HasValue() {
        return !(this->source.empty() && this->specifiedVersion.Empty());
    }

    string GetExactVersion() {
        return this->specifiedVersion.exact;
    }
};


struct lock_dependency {
    string localPath;
    string resolvedSource;
    string resolvedVersion;

    bool HasValue() {
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
