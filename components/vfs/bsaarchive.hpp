#ifndef VFS_BSAARCHIVE_HPP_
#define VFS_BSAARCHIVE_HPP_

#include "archive.hpp"
#include "file.hpp"
#include "pathutil.hpp"

#include <components/bsa/ba2dx10file.hpp>
#include <components/bsa/ba2gnrlfile.hpp>
#include <components/bsa/bsafile.hpp>
#include <components/bsa/compressedbsafile.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace VFS
{
    template <typename FileType>
    class BsaArchiveFile : public File
    {
    public:
        BsaArchiveFile(const Bsa::BSAFile::FileStruct* info, FileType* bsa)
            : mInfo(info)
            , mFile(bsa)
        {
        }

        Files::IStreamPtr open() override { return mFile->getFile(mInfo); }

        std::filesystem::path getPath() override { return mInfo->name(); }

        const Bsa::BSAFile::FileStruct* mInfo;
        FileType* mFile;
    };

    template <typename BSAFileType>
    class BsaArchive : public Archive
    {
    public:
        BsaArchive(const std::filesystem::path& filename)
            : Archive()
        {
            mFile = std::make_unique<BSAFileType>();
            mFile->open(filename);

            const Bsa::BSAFile::FileList& filelist = mFile->getList();
            for (Bsa::BSAFile::FileList::const_iterator it = filelist.begin(); it != filelist.end(); ++it)
            {
                mResources.emplace_back(&*it, mFile.get());
                mFiles.emplace_back(it->name());
            }

            std::sort(mFiles.begin(), mFiles.end());
        }

        void listResources(FileMap& out) override
        {
            for (auto& resource : mResources)
                out[VFS::Path::Normalized(resource.mInfo->name())] = &resource;
        }

        bool contains(Path::NormalizedView file) const override
        {
            return std::binary_search(mFiles.begin(), mFiles.end(), file);
        }

        std::string getDescription() const override { return std::string{ "BSA: " } + mFile->getFilename(); }

    private:
        std::unique_ptr<BSAFileType> mFile;
        std::vector<BsaArchiveFile<BSAFileType>> mResources;
        std::vector<VFS::Path::Normalized> mFiles;
    };

    inline std::unique_ptr<VFS::Archive> makeBsaArchive(const std::filesystem::path& path)
    {
        switch (Bsa::BSAFile::detectVersion(path))
        {
            case Bsa::BsaVersion::Unknown:
                break;
            case Bsa::BsaVersion::Uncompressed:
                return std::make_unique<BsaArchive<Bsa::BSAFile>>(path);
            case Bsa::BsaVersion::Compressed:
                return std::make_unique<BsaArchive<Bsa::CompressedBSAFile>>(path);
            case Bsa::BsaVersion::BA2GNRL:
                return std::make_unique<BsaArchive<Bsa::BA2GNRLFile>>(path);
            case Bsa::BsaVersion::BA2DX10:
                return std::make_unique<BsaArchive<Bsa::BA2DX10File>>(path);
        }

        throw std::runtime_error("Unknown archive type '" + Files::pathToUnicodeString(path) + "'");
    }
}

#endif
