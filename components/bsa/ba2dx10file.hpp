#ifndef OPENMW_COMPONENTS_BSA_BA2DX10FILE_HPP
#define OPENMW_COMPONENTS_BSA_BA2DX10FILE_HPP

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
            uint32_t mSize = 0;
            uint32_t mPackedSize = 0;
            int64_t mOffset = 0;
            uint16_t mStartMip = 0;
            uint16_t mEndMip = 0;
        };

        struct FileRecord
        {
            uint8_t mUnknownTex = 0;
            uint16_t mHeight = 0;
            uint16_t mWidth = 0;
            uint8_t mNumMips = 0;
            uint8_t mDXGIFormat = 0;
            uint16_t mCubeMaps = 0;
            std::vector<TextureChunkRecord> mTextureChunks;
        };

        uint32_t mVersion{ 0u };

        using FolderRecord = std::map<std::pair<uint32_t, uint32_t>, FileRecord>;
        std::map<uint32_t, FolderRecord> mFolders;

        std::list<std::vector<char>> mFileNames;

        std::optional<FileRecord> getFileRecord(std::string_view str) const;

        Files::IStreamPtr getFile(const FileRecord& fileRecord);

        void loadFiles(uint32_t fileCount, std::istream& in);

    public:
        using BSAFile::getFilename;
        using BSAFile::getList;
        using BSAFile::getPath;
        using BSAFile::open;

        BA2DX10File();
        virtual ~BA2DX10File();

        /// Read header information from the input source
        void readHeader(std::istream& stream) override;

        Files::IStreamPtr getFile(const FileStruct* fileStruct);
        void addFile(const std::string& filename, std::istream& file);
    };
}

#endif
