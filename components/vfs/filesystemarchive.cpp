#include "filesystemarchive.hpp"

#include <filesystem>

#include "pathutil.hpp"

#include <components/debug/debuglog.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>

namespace VFS
{

    FileSystemArchive::FileSystemArchive(const std::filesystem::path& path)
        : mBuiltIndex(false)
        , mPath(path)
    {
    }

    void FileSystemArchive::listResources(FileMap& out)
    {
        if (!mBuiltIndex)
        {
            const auto str = mPath.u8string();
            size_t prefix = str.size();

            if (!mPath.empty() && str[prefix - 1] != '\\' && str[prefix - 1] != '/')
                ++prefix;

            for (const auto& i : std::filesystem::recursive_directory_iterator(mPath))
            {
                if (std::filesystem::is_directory(i))
                    continue;

                const auto& path = i.path();
                const std::string proper = Files::pathToUnicodeString(path);

                FileSystemArchiveFile file(path);

                std::string searchable = Path::normalizeFilename(std::string_view{ proper }.substr(prefix));

                const auto inserted = mIndex.emplace(searchable, file);
                if (!inserted.second)
                    Log(Debug::Warning)
                        << "Warning: found duplicate file for '" << proper
                        << "', please check your file system for two files with the same name in different cases.";
                else
                    out[inserted.first->first] = &inserted.first->second;
            }
            mBuiltIndex = true;
        }
        else
        {
            for (auto& [k, v] : mIndex)
                out[k] = &v;
        }
    }

    bool FileSystemArchive::contains(std::string_view file) const
    {
        return mIndex.find(file) != mIndex.end();
    }

    std::string FileSystemArchive::getDescription() const
    {
        return "DIR: " + Files::pathToUnicodeString(mPath);
    }

    // ----------------------------------------------------------------------------------

    FileSystemArchiveFile::FileSystemArchiveFile(const std::filesystem::path& path)
        : mPath(path)
    {
    }

    Files::IStreamPtr FileSystemArchiveFile::open()
    {
        return Files::openConstrainedFileStream(mPath);
    }

}
