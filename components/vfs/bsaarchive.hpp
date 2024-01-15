#ifndef VFS_BSAARCHIVE_HPP_
#define VFS_BSAARCHIVE_HPP_

#include "archive.hpp"
#include "file.hpp"
#include "pathutil.hpp"

#include <components/bsa/ba2dx10file.hpp>
#include <components/bsa/ba2gnrlfile.hpp>
#include <components/bsa/bsa_file.hpp>
#include <components/bsa/compressedbsafile.hpp>

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
            }
        }

        virtual ~BsaArchive() {}

        void listResources(FileMap& out) override
        {
            for (auto& resource : mResources)
            {
                std::string ent = resource.mInfo->name();
                Path::normalizeFilenameInPlace(ent);

                out[ent] = &resource;
            }
        }

        bool contains(std::string_view file) const override
        {
            for (const auto& it : mResources)
            {
                if (Path::pathEqual(file, it.mInfo->name()))
                    return true;
            }
            return false;
        }

        std::string getDescription() const override { return std::string{ "BSA: " } + mFile->getFilename(); }

    private:
        std::unique_ptr<BSAFileType> mFile;
        std::vector<BsaArchiveFile<BSAFileType>> mResources;
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
