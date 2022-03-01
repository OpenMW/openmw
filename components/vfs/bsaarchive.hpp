#ifndef VFS_BSAARCHIVE_HPP_
#define VFS_BSAARCHIVE_HPP_

#include "archive.hpp"

#include <components/bsa/bsa_file.hpp>
#include <components/bsa/compressedbsafile.hpp>

namespace VFS
{
    class BsaArchiveFile : public File
    {
    public:
        BsaArchiveFile(const Bsa::BSAFile::FileStruct* info, Bsa::BSAFile* bsa);

        Files::IStreamPtr open() override;

        const Bsa::BSAFile::FileStruct* mInfo;
        Bsa::BSAFile* mFile;
    };

    class CompressedBsaArchiveFile : public File
    {
    public:
        CompressedBsaArchiveFile(const Bsa::BSAFile::FileStruct* info, Bsa::CompressedBSAFile* bsa);

        Files::IStreamPtr open() override;

        const Bsa::BSAFile::FileStruct* mInfo;
        Bsa::CompressedBSAFile* mCompressedFile;
    };


    class BsaArchive : public Archive
    {
    public:
        BsaArchive(const std::string& filename);
        BsaArchive();
        virtual ~BsaArchive();
        void listResources(std::map<std::string, File*>& out, char (*normalize_function) (char)) override;
        bool contains(const std::string& file, char (*normalize_function) (char)) const override;
        std::string getDescription() const override;

    protected:
        std::unique_ptr<Bsa::BSAFile> mFile;
        std::vector<BsaArchiveFile> mResources;
    };

    class CompressedBsaArchive : public Archive
    {
    public:
        CompressedBsaArchive(const std::string& filename);
        virtual ~CompressedBsaArchive() {}
        void listResources(std::map<std::string, File*>& out, char (*normalize_function) (char)) override;
        bool contains(const std::string& file, char (*normalize_function) (char)) const override;
        std::string getDescription() const override;

    private:
        std::unique_ptr<Bsa::CompressedBSAFile> mCompressedFile;
        std::vector<CompressedBsaArchiveFile> mCompressedResources;
    };

}

#endif
