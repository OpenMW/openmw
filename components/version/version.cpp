#include "version.hpp"

#include <filesystem>
#include <fstream>

namespace Version
{

    Version getOpenmwVersion(const std::filesystem::path& resourcePath)
    {
        std::ifstream stream(resourcePath / "version");

        Version v;
        std::getline(stream, v.mVersion);
        std::getline(stream, v.mCommitHash);
        std::getline(stream, v.mTagHash);
        return v;
    }

    std::string Version::describe() const
    {
        std::string str = "OpenMW version " + mVersion;
        std::string rev = mCommitHash;
        if (!rev.empty())
        {
            rev = rev.substr(0, 10);
            str += "\nRevision: " + rev;
        }
        return str;
    }

    std::string getOpenmwVersionDescription(const std::filesystem::path& resourcePath)
    {
        Version v = getOpenmwVersion(resourcePath);
        return v.describe();
    }

}
