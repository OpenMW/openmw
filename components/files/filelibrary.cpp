#include "filelibrary.hpp"

#include <iostream>

#include <boost/filesystem.hpp>
#include <../components/misc/stringops.hpp>

namespace Files
{
    // Looks for a string in a vector of strings
    bool containsVectorString(const StringVector& list, const std::string& str)
    {
        for (StringVector::const_iterator iter = list.begin();
             iter != list.end(); ++iter)
        {
            if (*iter == str)
                return true;
        }
        return false;
    }

    // Searches a path and adds the results to the library
    void FileLibrary::add(const boost::filesystem::path &root, bool recursive, bool strict,
            const StringVector &acceptableExtensions)
    {
        if (!boost::filesystem::exists(root))
        {
            std::cout << "Warning " << root.string() << " does not exist.\n";
            return;
        }

        std::string fileExtension;
        std::string type;

        // remember the last location of the priority list when listing new items
        int length = mPriorityList.size();

        // First makes a list of all candidate files
        FileLister(root, mPriorityList, recursive);

        // Then sort these files into sections according to the folder they belong to
        for (PathContainer::iterator listIter = mPriorityList.begin() + length;
            listIter != mPriorityList.end(); ++listIter)
        {
            if( !acceptableExtensions.empty() )
            {
                fileExtension = boost::filesystem::path (listIter->extension()).string();
                Misc::StringUtils::toLower(fileExtension);
                if(!containsVectorString(acceptableExtensions, fileExtension))
                    continue;
            }

            type = boost::filesystem::path (listIter->parent_path().leaf()).string();
            if (!strict)
                Misc::StringUtils::toLower(type);

            mMap[type].push_back(*listIter);
            // std::cout << "Added path: " << listIter->string() << " in section "<< type <<std::endl;
        }
    }

    // Returns true if the named section exists
    bool FileLibrary::containsSection(std::string sectionName, bool strict)
    {
        if (!strict)
            Misc::StringUtils::toLower(sectionName);
        StringPathContMap::const_iterator mapIter = mMap.find(sectionName);
        if (mapIter == mMap.end())
            return false;
        else
            return true;
    }

    // Returns a pointer to const for a section of the library
    const PathContainer* FileLibrary::section(std::string sectionName, bool strict)
    {
        if (!strict)
            Misc::StringUtils::toLower(sectionName);
        StringPathContMap::const_iterator mapIter = mMap.find(sectionName);
        if (mapIter == mMap.end())
        {
            //std::cout << "Empty\n";
            return &mEmptyPath;
        }
        else
        {
            return &(mapIter->second);
        }
    }

    // Searches the library for an item and returns a boost path to it
    boost::filesystem::path FileLibrary::locate(std::string item, bool strict, bool ignoreExtensions, std::string sectionName)
    {
        boost::filesystem::path result("");
        if (sectionName == "")
        {
            return FileListLocator(mPriorityList, boost::filesystem::path(item), strict, ignoreExtensions);
        }
        else
        {
            if (!containsSection(sectionName, strict))
            {
                std::cout << "Warning: There is no section named " << sectionName << "\n";
                return result;
            }
            result = FileListLocator(mMap[sectionName], boost::filesystem::path(item), strict, ignoreExtensions);
        }
        return result;
    }

    // Prints all the available sections, used for debugging
    void FileLibrary::printSections()
    {
        for(StringPathContMap::const_iterator mapIter = mMap.begin();
             mapIter != mMap.end(); ++mapIter)
        {
            std::cout << mapIter->first <<std::endl;
        }
    }
}
