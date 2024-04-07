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

        std::filesystem::recursive_directory_iterator iterator(mPath);

        for (auto it = std::filesystem::begin(iterator), end = std::filesystem::end(iterator); it != end;)
        {
            const std::filesystem::directory_entry& entry = *it;

            if (!entry.is_directory())
            {
                const std::filesystem::path& filePath = entry.path();
                const std::string proper = Files::pathToUnicodeString(filePath);
                VFS::Path::Normalized searchable(std::string_view{ proper }.substr(prefix));
                FileSystemArchiveFile file(filePath);

                const auto inserted = mIndex.emplace(std::move(searchable), std::move(file));
                if (!inserted.second)
                    Log(Debug::Warning)
                        << "Found duplicate file for '" << proper
                        << "', please check your file system for two files with the same name in different cases.";
            }

            // Exception thrown by the operator++ may not contain the context of the error like what exact path caused
            // the problem which makes it hard to understand what's going on when iteration happens over a directory
            // with thousands of files and subdirectories.
            const std::filesystem::path prevPath = entry.path();
            std::error_code ec;
            it.increment(ec);
            if (ec != std::error_code())
                throw std::runtime_error("Failed to recursively iterate over \"" + Files::pathToUnicodeString(mPath)
                    + "\" when incrementing to the next item from \"" + Files::pathToUnicodeString(prevPath)
                    + "\": " + ec.message());
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
