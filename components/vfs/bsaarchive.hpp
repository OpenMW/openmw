#ifndef VFS_BSAARCHIVE_HPP_
#define VFS_BSAARCHIVE_HPP_

#include "archive.hpp"

#include <components/bsa/bsa_file.hpp>

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

    class BsaArchive : public Archive
    {
    public:
        BsaArchive(const std::string& filename);
        virtual ~BsaArchive();
        void listResources(std::map<std::string, File*>& out, char (*normalize_function) (char)) override;

    private:
        std::unique_ptr<Bsa::BSAFile> mFile;
        std::vector<BsaArchiveFile> mResources;
    };
}

#endif
