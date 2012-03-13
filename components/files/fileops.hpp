#ifndef COMPONENTS_FILES_FILEOPS_HPP
#define COMPONENTS_FILES_FILEOPS_HPP

#include <map>
#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>

namespace Files
{

///\brief Check if a given path is an existing file (not a directory)
///\param [in] name - filename
bool isFile(const char *name);

    /// A vector of Boost Paths, very handy
    typedef std::vector<boost::filesystem::path> PathContainer;

    /// Makes a list of files from a directory by taking a boost
    /// path and a Path Container and adds to the Path container
    /// all files in the path. It has a recursive option.
    void FileLister( boost::filesystem::path currentPath, Files::PathContainer& list, bool recursive);

    /// Locates boost path in path container
    /// returns the path from the container
    /// that contains the searched path.
    /// If it's not found it returns and empty path
    /// Takes care of slashes, backslashes and it has a strict option.
    boost::filesystem::path FileListLocator (const Files::PathContainer& list, const boost::filesystem::path& toFind, bool strict);

    /// Overloaded form of the locator that takes a string and returns a string
    std::string FileListLocator (const Files::PathContainer& list,const std::string& toFind, bool strict);

}

#endif /* COMPONENTS_FILES_FILEOPS_HPP */
