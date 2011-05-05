
#include "collections.hpp"

namespace Files
{
    Collections::Collections() : mFoldCase (false) {}

    Collections::Collections (const std::vector<boost::filesystem::path>& directories, bool foldCase)
    : mDirectories (directories), mFoldCase (foldCase)
    {}

    const MultiDirCollection& Collections::getCollection (const std::string& extension) const
    {
        std::map<std::string, MultiDirCollection>::iterator iter = mCollections.find (extension);

        if (iter==mCollections.end())
        {
            std::pair<std::map<std::string, MultiDirCollection>::iterator, bool> result =
                mCollections.insert (std::make_pair (extension,
                MultiDirCollection (mDirectories, extension, mFoldCase)));

            iter = result.first;
        }

        return iter->second;
    }
}
