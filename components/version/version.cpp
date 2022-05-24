#include "version.hpp"

#include <filesystem>
#include <fstream>

namespace Version
{

Version getOpenmwVersion(const std::string &resourcePath)
{
    std::filesystem::path path (resourcePath + "/version");

    std::ifstream stream (path);

    Version v;
    std::getline(stream, v.mVersion);
    std::getline(stream, v.mCommitHash);
    std::getline(stream, v.mTagHash);
    return v;
}

std::string Version::describe()
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

std::string getOpenmwVersionDescription(const std::string &resourcePath)
{
    Version v = getOpenmwVersion(resourcePath);
    return v.describe();
}

}
