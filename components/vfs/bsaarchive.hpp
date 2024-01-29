#ifndef VFS_BSAARCHIVE_HPP_
#define VFS_BSAARCHIVE_HPP_

#include "archive.hpp"
#include "file.hpp"
#include "pathutil.hpp"

#include <components/bsa/ba2dx10file.hpp>
#include <components/bsa/ba2gnrlfile.hpp>
#include <components/bsa/bsa_file.hpp>
#include <components/bsa/compressedbsafile.hpp>

#include <algorithm>

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

        virtual ~BsaArchive() {}

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

    template <Bsa::BsaVersion>
    struct ArchiveSelector
    {
    };

    template <>
    struct ArchiveSelector<Bsa::BSAVER_UNCOMPRESSED>
    {
        using type = BsaArchive<Bsa::BSAFile>;
    };

    template <>
    struct ArchiveSelector<Bsa::BSAVER_COMPRESSED>
    {
        using type = BsaArchive<Bsa::CompressedBSAFile>;
    };

    template <>
    struct ArchiveSelector<Bsa::BSAVER_BA2_GNRL>
    {
        using type = BsaArchive<Bsa::BA2GNRLFile>;
    };

    template <>
    struct ArchiveSelector<Bsa::BSAVER_BA2_DX10>
    {
        using type = BsaArchive<Bsa::BA2DX10File>;
    };
}

#endif
