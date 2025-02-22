#ifndef BSA_BA2_DX10_FILE_H
#define BSA_BA2_DX10_FILE_H

#include <list>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bsafile.hpp"

namespace Bsa
{
    class BA2DX10File : private BSAFile
    {
    private:
        struct TextureChunkRecord
        {
            uint32_t size = 0;
            uint32_t packedSize = 0;
            int64_t offset = 0;
            uint16_t startMip = 0;
            uint16_t endMip = 0;
        };

        struct FileRecord
        {
            uint8_t unknownTex = 0;
            uint16_t height = 0;
            uint16_t width = 0;
            uint8_t numMips = 0;
            uint8_t DXGIFormat = 0;
            uint16_t cubeMaps = 0;
            std::vector<TextureChunkRecord> texturesChunks;
        };

        uint32_t mVersion{ 0u };

        using FolderRecord = std::map<std::pair<uint32_t, uint32_t>, FileRecord>;
        std::map<uint32_t, FolderRecord> mFolders;

        std::list<std::vector<char>> mFileNames;

        std::optional<FileRecord> getFileRecord(const std::string& str) const;

        Files::IStreamPtr getFile(const FileRecord& fileRecord);

        void loadFiles(uint32_t fileCount, std::istream& in);

    public:
        using BSAFile::getFilename;
        using BSAFile::getList;
        using BSAFile::open;

        BA2DX10File();
        virtual ~BA2DX10File();

        /// Read header information from the input source
        void readHeader() override;

        Files::IStreamPtr getFile(const char* filePath);
        Files::IStreamPtr getFile(const FileStruct* fileStruct);
        void addFile(const std::string& filename, std::istream& file);
    };
}

#endif
