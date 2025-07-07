#ifndef OPENMW_COMPONENTS_BSA_BA2GNRLFILE_HPP
#define OPENMW_COMPONENTS_BSA_BA2GNRLFILE_HPP

#include <list>
#include <map>
#include <string>
#include <vector>

#include "bsafile.hpp"

namespace Bsa
{
    class BA2GNRLFile : private BSAFile
    {
    private:
        struct FileRecord
        {
            FileRecord();
            uint32_t size;
            uint32_t offset;
            uint32_t packedSize = 0;
            bool isValid() const;
        };

        uint32_t mVersion{ 0u };

        using FolderRecord = std::map<std::pair<uint32_t, uint32_t>, FileRecord>;
        std::map<uint32_t, FolderRecord> mFolders;

        std::list<std::vector<char>> mFileNames;

        FileRecord getFileRecord(const std::string& str) const;

        Files::IStreamPtr getFile(const FileRecord& fileRecord);

        void loadFiles(uint32_t fileCount, std::istream& in);

    public:
        using BSAFile::getFilename;
        using BSAFile::getList;
        using BSAFile::getPath;
        using BSAFile::open;

        BA2GNRLFile();
        virtual ~BA2GNRLFile();

        /// Read header information from the input source
        void readHeader() override;

        Files::IStreamPtr getFile(const char* filePath);
        Files::IStreamPtr getFile(const FileStruct* fileStruct);
        void addFile(const std::string& filename, std::istream& file);
    };
}

#endif
