
#include "multidircollection.hpp"

#include <cctype>

#include <algorithm>
#include <stdexcept>
#include <iostream>

#include <boost/filesystem.hpp>

namespace Files
{
    struct NameEqual
    {
        bool mStrict;

        NameEqual (bool strict) : mStrict (strict) {}

        bool operator() (const std::string& left, const std::string& right) const
        {
            if (mStrict)
                return left==right;

            std::size_t len = left.length();

            if (len!=right.length())
                return false;

            for (std::size_t i=0; i<len; ++i)
            {
                char l = std::tolower (left[i]);
                char r = std::tolower (right[i]);

                if (l!=r)
                    return false;
            }

            return true;
        }
    };

    MultiDirCollection::MultiDirCollection(const Files::PathContainer& directories,
        const std::string& extension, bool foldCase)
    : mFiles (NameLess (!foldCase))
    {
        NameEqual equal (!foldCase);

        for (PathContainer::const_iterator iter = directories.begin();
            iter!=directories.end(); ++iter)
        {
            if (!boost::filesystem::is_directory(*iter))
            {
                std::cout << "Skipping invalid directory: " << (*iter).string() << std::endl;
                continue;
            }

            for (boost::filesystem::directory_iterator dirIter(*iter);
                    dirIter != boost::filesystem::directory_iterator(); ++dirIter)
            {
                boost::filesystem::path path = *dirIter;

                if (!equal (extension, boost::filesystem::path (path.extension()).string()))
                    continue;

                std::string filename = boost::filesystem::path (path.filename()).string();

                TIter result = mFiles.find (filename);

                if (result==mFiles.end())
                {
                    mFiles.insert (std::make_pair (filename, path));
                }
                else if (result->first==filename)
                {
                    mFiles[filename] = path;
                }
                else
                {
                    // handle case folding
                    mFiles.erase (result->first);
                    mFiles.insert (std::make_pair (filename, path));
                }
            }
        }
    }

    boost::filesystem::path MultiDirCollection::getPath (const std::string& file) const
    {
        TIter iter = mFiles.find (file);

        if (iter==mFiles.end())
            throw std::runtime_error ("file " + file + " not found");

        return iter->second;
    }

    bool MultiDirCollection::doesExist (const std::string& file) const
    {
        return mFiles.find (file)!=mFiles.end();
    }

    MultiDirCollection::TIter MultiDirCollection::begin() const
    {
        return mFiles.begin();
    }

    MultiDirCollection::TIter MultiDirCollection::end() const
    {
        return mFiles.end();
    }
}
