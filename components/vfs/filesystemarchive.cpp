#include "filesystemarchive.hpp"

#include <algorithm>

#include <filesystem>

#include <components/debug/debuglog.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>

namespace VFS
{

    FileSystemArchive::FileSystemArchive(const std::filesystem::path &path)
        : mBuiltIndex(false)
        , mPath(path)
    {

    }

    void FileSystemArchive::listResources(std::map<std::string, File *> &out, char (*normalize_function)(char))
    {
        if (!mBuiltIndex)
        {
            const auto str = mPath.u8string();
            size_t prefix = str.size ();

            if (!mPath.empty() && str [prefix - 1] != '\\' && str [prefix - 1] != '/')
                ++prefix;

            for (const auto& i :
                    std::filesystem::recursive_directory_iterator(mPath))
            {
                if(std::filesystem::is_directory (i))
                    continue;

                const auto& path = i.path ();
                const auto& proper = Files::pathToUnicodeString(path);

                FileSystemArchiveFile file(path);

                std::string searchable;

                std::transform(std::next(proper.begin(), static_cast<std::string::difference_type>(prefix)), proper.end(), std::back_inserter(searchable), normalize_function);

                const auto inserted = mIndex.insert(std::make_pair(searchable, file));
                if (!inserted.second)
                    Log(Debug::Warning) << "Warning: found duplicate file for '" << proper << "', please check your file system for two files with the same name in different cases.";
                else
                    out[inserted.first->first] = &inserted.first->second;
            }
            mBuiltIndex = true;
        }
        else
        {
            for (index::iterator it = mIndex.begin(); it != mIndex.end(); ++it)
            {
                out[it->first] = &it->second;
            }
        }
    }

    bool FileSystemArchive::contains(const std::string& file, char (*normalize_function)(char)) const
    {
        return mIndex.find(file) != mIndex.end();
    }

    std::string FileSystemArchive::getDescription() const
    {
        return "DIR: " + Files::pathToUnicodeString(mPath);
    }

    // ----------------------------------------------------------------------------------

    FileSystemArchiveFile::FileSystemArchiveFile(const std::filesystem::path &path)
        : mPath(path)
    {
    }

    Files::IStreamPtr FileSystemArchiveFile::open()
    {
        return Files::openConstrainedFileStream(mPath);
    }

}
