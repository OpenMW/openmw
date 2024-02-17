#include "filesystemarchive.hpp"

#include <filesystem>

#include "pathutil.hpp"

#include <components/debug/debuglog.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>

namespace VFS
{

    FileSystemArchive::FileSystemArchive(const std::filesystem::path& path)
        : mPath(path)
    {
        const auto str = mPath.u8string();
        std::size_t prefix = str.size();

        if (prefix > 0 && str[prefix - 1] != '\\' && str[prefix - 1] != '/')
            ++prefix;

        for (const auto& i : std::filesystem::recursive_directory_iterator(mPath))
        {
            if (std::filesystem::is_directory(i))
                continue;

            const std::filesystem::path& filePath = i.path();
            const std::string proper = Files::pathToUnicodeString(filePath);
            VFS::Path::Normalized searchable(std::string_view{ proper }.substr(prefix));
            FileSystemArchiveFile file(filePath);

            const auto inserted = mIndex.emplace(std::move(searchable), std::move(file));
            if (!inserted.second)
                Log(Debug::Warning)
                    << "Found duplicate file for '" << proper
                    << "', please check your file system for two files with the same name in different cases.";
        }
    }

    void FileSystemArchive::listResources(FileMap& out)
    {
        for (auto& [k, v] : mIndex)
            out[k] = &v;
    }

    bool FileSystemArchive::contains(Path::NormalizedView file) const
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
