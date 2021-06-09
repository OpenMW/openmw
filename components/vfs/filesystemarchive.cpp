#include "filesystemarchive.hpp"

#include <boost/filesystem.hpp>

#include <components/debug/debuglog.hpp>

namespace VFS
{

    FileSystemArchive::FileSystemArchive(const std::string &path)
        : mBuiltIndex(false)
        , mPath(path)
    {

    }

    void FileSystemArchive::listResources(std::map<std::string, File *> &out, char (*normalize_function)(char))
    {
        if (!mBuiltIndex)
        {
            typedef boost::filesystem::recursive_directory_iterator directory_iterator;

            directory_iterator end;

            size_t prefix = mPath.size ();

            if (mPath.size () > 0 && mPath [prefix - 1] != '\\' && mPath [prefix - 1] != '/')
                ++prefix;

            for (directory_iterator i (mPath); i != end; ++i)
            {
                if(boost::filesystem::is_directory (*i))
                    continue;

                std::string proper = i->path ().string ();

                FileSystemArchiveFile file(proper);

                std::string searchable;

                std::transform(proper.begin() + prefix, proper.end(), std::back_inserter(searchable), normalize_function);

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
        return std::string{"DIR: "} + mPath;
    }

    // ----------------------------------------------------------------------------------

    FileSystemArchiveFile::FileSystemArchiveFile(const std::string &path)
        : mPath(path)
    {
    }

    Files::IStreamPtr FileSystemArchiveFile::open()
    {
        return Files::openConstrainedFileStream(mPath.c_str());
    }

}
