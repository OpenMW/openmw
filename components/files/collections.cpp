#include "collections.hpp"
#include "conversion.hpp"

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>

namespace Files
{
    Collections::Collections()
        : mDirectories()
        , mCollections()
    {
    }

    Collections::Collections(const Files::PathContainer& directories)
        : mDirectories(directories)
        , mCollections()
    {
    }

    const MultiDirCollection& Collections::getCollection(const std::string& extension) const
    {
        std::string ext = Misc::StringUtils::lowerCase(extension);
        auto iter = mCollections.find(ext);
        if (iter == mCollections.end())
        {
            std::pair<MultiDirCollectionContainer::iterator, bool> result
                = mCollections.emplace(ext, MultiDirCollection(mDirectories, ext));

            iter = result.first;
        }

        return iter->second;
    }

    std::filesystem::path Collections::getPath(const std::string& file) const
    {
        for (auto iter = mDirectories.rbegin(); iter != mDirectories.rend(); iter++)
        {
            for (const auto& iter2 : std::filesystem::directory_iterator(*iter))
            {
                const auto& path = iter2.path();
                const auto str = Files::pathToUnicodeString(path.filename());

                if (Misc::StringUtils::ciEqual(file, str))
                    return path;
            }
        }

        throw std::runtime_error("file " + file + " not found");
    }

    bool Collections::doesExist(const std::string& file) const
    {
        for (auto iter = mDirectories.rbegin(); iter != mDirectories.rend(); iter++)
        {
            for (const auto& iter2 : std::filesystem::directory_iterator(*iter))
            {
                const auto& path = iter2.path();
                const auto str = Files::pathToUnicodeString(path.filename());

                if (Misc::StringUtils::ciEqual(file, str))
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
