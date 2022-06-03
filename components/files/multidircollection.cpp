#include "multidircollection.hpp"

#include <boost/filesystem.hpp>

#include <components/debug/debuglog.hpp>

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
            return Misc::StringUtils::ciEqual(left, right);
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
                Log(Debug::Info) << "Skipping invalid directory: " << (*iter).string();
                continue;
            }

            for (boost::filesystem::directory_iterator dirIter(*iter);
                    dirIter != boost::filesystem::directory_iterator(); ++dirIter)
            {
                boost::filesystem::path path = *dirIter;

                if (!equal (extension, path.extension().string()))
                    continue;

                std::string filename = path.filename().string();

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
