#include "collections.hpp"
#include "conversion.hpp"

#include <components/misc/strings/algorithm.hpp>

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

    const MultiDirCollection& Collections::getCollection(std::string_view extension) const
    {
        auto iter = mCollections.find(extension);
        if (iter == mCollections.end())
        {
            auto result = mCollections.emplace(extension, MultiDirCollection(mDirectories, extension));

            iter = result.first;
        }

        return iter->second;
    }

    std::filesystem::path Collections::getPath(std::string_view file) const
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

        throw std::runtime_error("file " + std::string(file) + " not found");
    }

    bool Collections::doesExist(std::string_view file) const
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
