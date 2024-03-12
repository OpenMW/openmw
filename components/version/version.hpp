#ifndef VERSION_HPP
#define VERSION_HPP

#include <filesystem>
#include <string>
#include <string_view>

namespace Version
{
    std::string_view getVersion();
    std::string_view getCommitHash();
    std::string_view getTagHash();
    int getLuaApiRevision();
    int getPostprocessingApiRevision();

    // Prepares string that contains version and commit hash.
    std::string getOpenmwVersionDescription();

    bool checkResourcesVersion(const std::filesystem::path& resourcePath);

    std::string_view getDocumentationUrl();
}

#endif // VERSION_HPP
