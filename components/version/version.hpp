#ifndef VERSION_HPP
#define VERSION_HPP

#include <string>
#include <filesystem>

namespace Version
{

    struct Version
    {
        std::string mVersion;
        std::string mCommitHash;
        std::string mTagHash;

        std::string describe();
    };

    /// Read OpenMW version from the version file located in resourcePath.
    Version getOpenmwVersion(const std::filesystem::path &resourcePath);

    /// Helper function to getOpenmwVersion and describe() it
    std::string getOpenmwVersionDescription(const std::filesystem::path &resourcePath);

}


#endif // VERSION_HPP

