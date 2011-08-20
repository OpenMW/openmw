
#include "collections.hpp"

namespace Files
{
    Collections::Collections()
        : mDirectories()
        , mFoldCase(false)
        , mCollections()
    {
    }

    Collections::Collections(const std::vector<boost::filesystem::path>& directories, bool foldCase)
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
}
