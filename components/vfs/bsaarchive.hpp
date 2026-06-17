#ifndef VFS_BSAARCHIVE_HPP_
#define VFS_BSAARCHIVE_HPP_

#include "archive.hpp"
#include "file.hpp"
#include "pathutil.hpp"

#include <components/bsa/ba2dx10file.hpp>
#include <components/bsa/ba2gnrlfile.hpp>
#include <components/bsa/bsafile.hpp>
#include <components/bsa/compressedbsafile.hpp>

#include <components/toutf8/toutf8.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace VFS
{
    template <typename BSAFileType>
    class BsaArchive;

    template <typename FileType>
    class BsaArchiveFile : public File
    {
    public:
        BsaArchiveFile(const Bsa::BSAFile::FileStruct* info, const BsaArchive<FileType>* bsa)
            : mInfo(info)
            , mFile(bsa)
        {
        }

        Files::IStreamPtr open() override { return mFile->getFile()->getFile(mInfo); }

        std::filesystem::file_time_type getLastModified() const override
        {
            return std::filesystem::last_write_time(mFile->getFile()->getPath());
        }

        std::string getStem() const override
        {
            std::string_view name = mInfo->name();
            auto index = name.find_last_of("\\/");
            if (index != std::string_view::npos)
                name = name.substr(index + 1);
            index = name.find_last_of('.');
            if (index != std::string_view::npos && index != 0)
                name = name.substr(0, index);
            std::string out;
            std::string_view utf8 = mFile->getUtf8(name, out);
            if (out.data() == utf8.data())
                out.resize(utf8.size());
            else
                out = utf8;
            return out;
        }

        const Bsa::BSAFile::FileStruct* mInfo;
        const BsaArchive<FileType>* mFile;
    };

    template <typename BSAFileType>
    class BsaArchive : public Archive
    {
    public:
        BsaArchive(const std::filesystem::path& filename, const ToUTF8::StatelessUtf8Encoder* encoder)
            : Archive()
            , mEncoder(encoder)
        {
            mFile = std::make_unique<BSAFileType>();
            mFile->open(filename);

            std::string buffer;
            for (const Bsa::BSAFile::FileStruct& file : mFile->getList())
            {
                mResources.emplace_back(&file, this);
                mFiles.emplace_back(getUtf8(file.name(), buffer));
            }

            std::sort(mFiles.begin(), mFiles.end());
        }

        void listResources(FileMap& out) override
        {
            std::string buffer;
            for (auto& resource : mResources)
            {
                std::string_view path = getUtf8(resource.mInfo->name(), buffer);
                out[VFS::Path::Normalized(path)] = &resource;
            }
        }

        bool contains(Path::NormalizedView file) const override
        {
            return std::binary_search(mFiles.begin(), mFiles.end(), file);
        }

        std::string getDescription() const override { return std::string{ "BSA: " } + mFile->getFilename(); }

        BSAFileType* getFile() const { return mFile.get(); }

        std::string_view getUtf8(std::string_view input, std::string& buffer) const
        {
            if (mEncoder == nullptr)
                return input;
            return mEncoder->getUtf8(input, ToUTF8::BufferAllocationPolicy::UseGrowFactor, buffer);
        }

    private:
        std::unique_ptr<BSAFileType> mFile;
        std::vector<BsaArchiveFile<BSAFileType>> mResources;
        std::vector<VFS::Path::Normalized> mFiles;
        const ToUTF8::StatelessUtf8Encoder* mEncoder;
    };

    inline std::unique_ptr<VFS::Archive> makeBsaArchive(
        const std::filesystem::path& path, const ToUTF8::StatelessUtf8Encoder* encoder)
    {
        switch (Bsa::BSAFile::detectVersion(path))
        {
            case Bsa::BsaVersion::Unknown:
                break;
            case Bsa::BsaVersion::Uncompressed:
                return std::make_unique<BsaArchive<Bsa::BSAFile>>(path, encoder);
            case Bsa::BsaVersion::Compressed:
                return std::make_unique<BsaArchive<Bsa::CompressedBSAFile>>(path, encoder);
            case Bsa::BsaVersion::BA2GNRL:
                return std::make_unique<BsaArchive<Bsa::BA2GNRLFile>>(path, encoder);
            case Bsa::BsaVersion::BA2DX10:
                return std::make_unique<BsaArchive<Bsa::BA2DX10File>>(path, encoder);
        }

        throw std::runtime_error("Unknown archive type '" + Files::pathToUnicodeString(path) + "'");
    }
}

#endif
