#include "collections.hpp"
#include "conversion.hpp"

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>

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
        std::string ext = Misc::StringUtils::lowerCase(extension);
        auto iter = mCollections.find(ext);
        if (iter==mCollections.end())
        {
            std::pair<MultiDirCollectionContainer::iterator, bool> result =
                mCollections.emplace(ext, MultiDirCollection(mDirectories, ext, mFoldCase));

            iter = result.first;
        }

        return iter->second;
    }

    std::filesystem::path Collections::getPath(const std::string& file) const
    {
        for (const auto & mDirectorie : mDirectories)
        {
            for (const auto& iter2 :
                std::filesystem::directory_iterator (mDirectorie))
            {
                const auto& path = iter2.path();
                const auto str = Files::pathToUnicodeString(path.filename());

                if (mFoldCase)
                {
                    if (Misc::StringUtils::ciEqual(file, str))
                        return path;
                }
                else if (str == file)
                    return path;
            }
        }

        throw std::runtime_error ("file " + file + " not found");
    }

    bool Collections::doesExist(const std::string& file) const
    {
        for (const auto & mDirectorie : mDirectories)
        {
            for (const auto& iter2 :
                std::filesystem::directory_iterator (mDirectorie))
            {
                const auto& path = iter2.path();
                const auto str = Files::pathToUnicodeString(path.filename());

                if (mFoldCase)
                {
                    if (Misc::StringUtils::ciEqual(file, str))
                        return true;
                }
                else if (str == file)
                    return true;
            }
        }

        return false;
    }

    const Files::PathContainer& Collections::getPaths() const
    {
        return mDirectories;
    }
}
