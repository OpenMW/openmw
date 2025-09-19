#include "ba2gnrlfile.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>

#include <zlib.h>

#include <components/esm/fourcc.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/lower.hpp>

#include "ba2file.hpp"
#include "memorystream.hpp"

namespace Bsa
{
    // special marker for invalid records,
    const uint32_t sInvalidOffset = std::numeric_limits<uint32_t>::max();

    BA2GNRLFile::FileRecord::FileRecord()
        : size(0)
        , offset(sInvalidOffset)
    {
    }

    bool BA2GNRLFile::FileRecord::isValid() const
    {
        return offset != sInvalidOffset;
    }

    BA2GNRLFile::BA2GNRLFile() {}

    BA2GNRLFile::~BA2GNRLFile() = default;

    void BA2GNRLFile::loadFiles(uint32_t fileCount, std::istream& in)
    {
        mFiles.clear();
        mFiles.reserve(fileCount);
        for (uint32_t i = 0; i < fileCount; ++i)
        {
            uint32_t nameHash, extHash, dirHash;
            in.read(reinterpret_cast<char*>(&nameHash), sizeof(uint32_t));
            in.read(reinterpret_cast<char*>(&extHash), sizeof(uint32_t));
            in.read(reinterpret_cast<char*>(&dirHash), sizeof(uint32_t));

            FileRecord file;
            uint32_t unknown;
            in.read(reinterpret_cast<char*>(&unknown), sizeof(uint32_t));
            in.read(reinterpret_cast<char*>(&file.offset), sizeof(int64_t));
            in.read(reinterpret_cast<char*>(&file.packedSize), sizeof(uint32_t));
            in.read(reinterpret_cast<char*>(&file.size), sizeof(uint32_t));

            uint32_t baadfood;
            in.read(reinterpret_cast<char*>(&baadfood), sizeof(uint32_t));
            if (baadfood != 0xBAADF00D)
                fail("Corrupted BSA");

            mFolders[dirHash][{ nameHash, extHash }] = file;

            FileStruct fileStruct{};
            fileStruct.mFileSize = file.size;
            fileStruct.mOffset = file.offset;
            mFiles.push_back(fileStruct);
        }
    }

    /// Read header information from the input source
    void BA2GNRLFile::readHeader()
    {
        assert(!mIsLoaded);

        std::ifstream input(mFilepath, std::ios_base::binary);

        // Total archive size
        std::streamoff fsize = 0;
        if (input.seekg(0, std::ios_base::end))
        {
            fsize = input.tellg();
            input.seekg(0);
        }

        if (fsize < 24) // header is 24 bytes
            fail("File too small to be a valid BSA archive");

        // Get essential header numbers
        uint32_t type, fileCount;
        uint64_t fileTableOffset;
        {
            uint32_t header[4];
            input.read(reinterpret_cast<char*>(header), 16);
            input.read(reinterpret_cast<char*>(&fileTableOffset), 8);

            if (header[0] != ESM::fourCC("BTDX"))
                fail("Unrecognized BA2 signature");
            mVersion = header[1];
            switch (static_cast<BA2Version>(mVersion))
            {
                case BA2Version::Fallout4:
                case BA2Version::Fallout4NextGen_v7:
                case BA2Version::Fallout4NextGen_v8:
                    break;
                case BA2Version::StarfieldGeneral:
                    uint64_t dummy;
                    input.read(reinterpret_cast<char*>(&dummy), 8);
                    break;
                default:
                    fail("Unrecognized general BA2 version");
            }

            type = header[2];
            fileCount = header[3];
        }

        if (type == ESM::fourCC("GNRL"))
            loadFiles(fileCount, input);
        else
            fail("Unrecognized ba2 version type");

        // Read the string table
        input.seekg(fileTableOffset);
        for (uint32_t i = 0; i < fileCount; ++i)
        {
            std::vector<char> fileName;
            uint16_t fileNameSize;
            input.read(reinterpret_cast<char*>(&fileNameSize), sizeof(uint16_t));
            fileName.resize(fileNameSize);
            input.read(fileName.data(), fileName.size());
            fileName.push_back('\0');
            mFileNames.push_back(std::move(fileName));
            mFiles[i].setNameInfos(0, &mFileNames.back());
        }

        mIsLoaded = true;
    }

    BA2GNRLFile::FileRecord BA2GNRLFile::getFileRecord(const std::string& str) const
    {
        for (const auto c : str)
        {
            if (((static_cast<unsigned>(c) >> 7U) & 1U) != 0U)
            {
                fail("File record " + str + " contains unicode characters, refusing to load.");
            }
        }

#ifdef _WIN32
        const auto& path = str;
#else
        // Force-convert the path into something UNIX can handle first
        // to make sure std::filesystem::path doesn't think the entire path is the filename on Linux
        // and subsequently purge it to determine the file folder.
        std::string path = str;
        std::replace(path.begin(), path.end(), '\\', '/');
#endif

        const auto p = std::filesystem::path{ path }; // Purposefully damage Unicode strings.
        const auto fileName = Misc::StringUtils::lowerCase(p.stem().string());
        const auto ext = Misc::StringUtils::lowerCase(p.extension().string()); // Purposefully damage Unicode strings.
        const auto folder = Misc::StringUtils::lowerCase(p.parent_path().string());

        uint32_t folderHash = generateHash(folder);
        auto it = mFolders.find(folderHash);
        if (it == mFolders.end())
            return FileRecord(); // folder not found, return default which has offset of sInvalidOffset

        uint32_t fileHash = generateHash(fileName);
        uint32_t extHash = generateExtensionHash(ext);
        auto iter = it->second.find({ fileHash, extHash });
        if (iter == it->second.end())
            return FileRecord(); // file not found, return default which has offset of sInvalidOffset
        return iter->second;
    }

    Files::IStreamPtr BA2GNRLFile::getFile(const FileStruct* file)
    {
        FileRecord fileRec = getFileRecord(file->name());
        if (!fileRec.isValid())
        {
            fail("File not found: " + std::string(file->name()));
        }
        return getFile(fileRec);
    }

    void BA2GNRLFile::addFile(const std::string& filename, std::istream& file)
    {
        assert(false); // not implemented yet
        fail("Add file is not implemented for compressed BSA: " + filename);
    }

    Files::IStreamPtr BA2GNRLFile::getFile(const char* file)
    {
        FileRecord fileRec = getFileRecord(file);
        if (!fileRec.isValid())
        {
            fail("File not found: " + std::string(file));
        }
        return getFile(fileRec);
    }

    Files::IStreamPtr BA2GNRLFile::getFile(const FileRecord& fileRecord)
    {
        const uint32_t inputSize = fileRecord.packedSize ? fileRecord.packedSize : fileRecord.size;
        Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilepath, fileRecord.offset, inputSize);
        auto memoryStreamPtr = std::make_unique<MemoryInputStream>(fileRecord.size);
        if (fileRecord.packedSize)
        {
            std::vector<char> buffer(inputSize);
            streamPtr->read(buffer.data(), inputSize);
            uLongf destSize = static_cast<uLongf>(fileRecord.size);
            int ec = ::uncompress(reinterpret_cast<Bytef*>(memoryStreamPtr->getRawData()), &destSize,
                reinterpret_cast<Bytef*>(buffer.data()), static_cast<uLong>(buffer.size()));

            if (ec != Z_OK)
                fail("zlib uncompress failed: " + std::string(::zError(ec)));
        }
        else
        {
            streamPtr->read(memoryStreamPtr->getRawData(), fileRecord.size);
        }
        return std::make_unique<Files::StreamWithBuffer<MemoryInputStream>>(std::move(memoryStreamPtr));
    }

} // namespace Bsa
