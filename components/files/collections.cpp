
#include "collections.hpp"

namespace Files
{
    Collections::Collections()
        : mDirectories()
        , mFoldCase(false)
        , mCollections()
    {
    }

    Collections::Collections(const Files::PathContainer& directories, bool foldCase)
        : mDirectories(directories)
        , mFoldCase(foldCase)
        , mCollections()
    {
    }

    const MultiDirCollection& Collections::getCollection(const std::string& extension) const
    {
        MultiDirCollectionContainer::iterator iter = mCollections.find(extension);
        if (iter==mCollections.end())
        {
            std::pair<MultiDirCollectionContainer::iterator, bool> result =
                mCollections.insert(std::make_pair(extension, MultiDirCollection(mDirectories, extension, mFoldCase)));

            iter = result.first;
        }

        return iter->second;
    }

    boost::filesystem::path Collections::getPath(const std::string& file) const
    {
        for (Files::PathContainer::const_iterator iter = mDirectories.begin();
             iter != mDirectories.end(); ++iter)
        {
            const boost::filesystem::path path = *iter / file;
            if (boost::filesystem::exists(path))
                return path.string();
        }

        throw std::runtime_error ("file " + file + " not found");
    }

    bool Collections::doesExist(const std::string& file) const
    {
        for (Files::PathContainer::const_iterator iter = mDirectories.begin();
             iter != mDirectories.end(); ++iter)
        {
            const boost::filesystem::path path = *iter / file;
            if (boost::filesystem::exists(path))
                return true;
        }

        return false;
    }

    const Files::PathContainer& Collections::getPaths() const
    {
        return mDirectories;
    }
}
