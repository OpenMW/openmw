#include "version.hpp"

#include <osgDB/fstream>

#include <boost/filesystem/path.hpp>

namespace Version
{

Version getOpenmwVersion(const std::string &resourcePath)
{
    boost::filesystem::path path (resourcePath + "/version");

    osgDB::ifstream stream (path.string().c_str());

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
