#include "version.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

namespace Version
{

Version getOpenmwVersion(const std::string &resourcePath)
{
    boost::filesystem::path path (resourcePath + "/version");

    boost::filesystem::ifstream stream (path);

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
