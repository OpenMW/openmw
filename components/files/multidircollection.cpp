#include "multidircollection.hpp"
#include "conversion.hpp"

#include <filesystem>

#include <components/debug/debuglog.hpp>

namespace Files
{

    MultiDirCollection::MultiDirCollection(const Files::PathContainer& directories, std::string_view extension)
    {
        for (const auto& directory : directories)
        {
            if (!std::filesystem::is_directory(directory))
            {
                Log(Debug::Info) << "Skipping invalid directory: " << directory;
                continue;
            }

            for (const auto& dirIter : std::filesystem::directory_iterator(directory))
            {
                const auto& path = dirIter.path();

                std::string ext = Files::pathToUnicodeString(path.extension());
                if (ext.size() != extension.size() + 1 || !Misc::StringUtils::ciEndsWith(ext, extension))
                    continue;

                const auto filename = Files::pathToUnicodeString(path.filename());

                TIter result = mFiles.find(filename);

                if (result == mFiles.end())
                {
                    mFiles.insert(std::make_pair(filename, path));
                }
                else if (result->first == filename)
                {
                    mFiles[filename] = path;
                }
                else
                {
                    // handle case folding
                    mFiles.erase(result->first);
                    mFiles.emplace(filename, path);
                }
            }
        }
    }

    std::filesystem::path MultiDirCollection::getPath(std::string_view file) const
    {
        TIter iter = mFiles.find(file);

        if (iter == mFiles.end())
            throw std::runtime_error("file " + std::string(file) + " not found");

        return iter->second;
    }

    bool MultiDirCollection::doesExist(std::string_view file) const
    {
        return mFiles.contains(file);
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
