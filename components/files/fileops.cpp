#include "fileops.hpp"

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <../components/misc/stringops.hpp>

namespace Files
{

bool isFile(const char *name)
{
    return boost::filesystem::exists(boost::filesystem::path(name));
}

    // Returns true if the last part of the superset matches the subset
    bool endingMatches(const std::string& superset, const std::string& subset)
    {
        if (subset.length() > superset.length())
            return false;
        return superset.substr(superset.length() - subset.length()) == subset;
    }

    // Makes a list of files from a directory
    void FileLister( boost::filesystem::path currentPath, Files::PathContainer& list, bool recursive)
    {
        if (!boost::filesystem::exists(currentPath))
        {
            std::cout << "WARNING: " << currentPath.string() << " does not exist.\n";
            return ;
        }
        if (recursive)
        {
            for ( boost::filesystem::recursive_directory_iterator end, itr(currentPath.string());
                itr != end; ++itr )
            {
                if ( boost::filesystem::is_regular_file(*itr))
                    list.push_back(itr->path());
            }
        }
        else
        {
            for ( boost::filesystem::directory_iterator end, itr(currentPath.string());
                itr != end; ++itr )
            {
                if ( boost::filesystem::is_regular_file(*itr))
                    list.push_back(itr->path());
            }
        }
    }

    // Locates path in path container
    boost::filesystem::path FileListLocator (const Files::PathContainer& list, const boost::filesystem::path& toFind,
                                             bool strict, bool ignoreExtensions)
    {
        boost::filesystem::path result("");
        if (list.empty())
            return result;

        std::string toFindStr;
        if (ignoreExtensions)
            toFindStr = boost::filesystem::basename(toFind);
        else
            toFindStr = toFind.string();

        std::string fullPath;

        // The filesystems slash sets the default slash
        std::string slash;
        std::string wrongslash;
        if(list[0].string().find("\\") != std::string::npos)
        {
            slash = "\\";
            wrongslash = "/";
        }
        else
        {
            slash = "/";
            wrongslash = "\\";
        }

        // The file being looked for is converted to the new slash
        if(toFindStr.find(wrongslash) != std::string::npos )
        {
            boost::replace_all(toFindStr, wrongslash, slash);
        }

        if (!strict)
        {
            Misc::StringUtils::toLower(toFindStr);
        }

        for (Files::PathContainer::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            fullPath = it->string();
            if (ignoreExtensions)
                fullPath.erase(fullPath.length() -
                    boost::filesystem::path (it->extension()).string().length());

            if (!strict)
            {
                Misc::StringUtils::toLower(fullPath);
            }
            if(endingMatches(fullPath, toFindStr))
            {
                result = *it;
                break;
            }
        }
        return result;
    }

    // Overloaded form of the locator that takes a string and returns a string
    std::string FileListLocator (const Files::PathContainer& list,const std::string& toFind, bool strict, bool ignoreExtensions)
    {
        return FileListLocator(list, boost::filesystem::path(toFind), strict, ignoreExtensions).string();
    }

}
