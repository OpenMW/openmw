/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (compressedbsafile.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

  Compressed BSA stuff added by cc9cii 2018

 */
#include "compressedbsafile.hpp"

#include <stdexcept>
#include <cassert>
#include <filesystem>
#include <fstream>

#include <lz4frame.h>


#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>

#if defined(_MSC_VER)
    #pragma warning (push)
    #pragma warning (disable : 4706)
    #include <boost/iostreams/filter/zlib.hpp>
    #pragma warning (pop)
#else
    #include <boost/iostreams/filter/zlib.hpp>
#endif

#include <boost/iostreams/device/array.hpp>
#include <components/bsa/memorystream.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/files/constrainedfilestream.hpp>

namespace Bsa
{
//special marker for invalid records,
//equal to max uint32_t value
const uint32_t CompressedBSAFile::sInvalidOffset = std::numeric_limits<uint32_t>::max();

//bit marking compression on file size
const uint32_t CompressedBSAFile::sCompressedFlag = 1u << 30u;


CompressedBSAFile::FileRecord::FileRecord() : size(0), offset(sInvalidOffset)
{ }

bool CompressedBSAFile::FileRecord::isValid() const
{
    return offset != sInvalidOffset;
}

bool CompressedBSAFile::FileRecord::isCompressed(bool bsaCompressedByDefault) const
{
    bool recordCompressionFlagEnabled = ((size & sCompressedFlag) == sCompressedFlag);

    //record is compressed when:
    //- bsaCompressedByDefault flag is set and 30th bit is NOT set, or
    //- bsaCompressedByDefault flag is NOT set and 30th bit is set
    //record is NOT compressed when:
    //- bsaCompressedByDefault flag is NOT set and 30th bit is NOT set, or
    //- bsaCompressedByDefault flag is set and 30th bit is set
    return (bsaCompressedByDefault != recordCompressionFlagEnabled);
}

std::uint32_t CompressedBSAFile::FileRecord::getSizeWithoutCompressionFlag() const {
    return size & (~sCompressedFlag);
}

void CompressedBSAFile::getBZString(std::string& str, std::istream& filestream)
{
    char size = 0;
    filestream.read(&size, 1);

    auto buf = std::vector<char>(size);
    filestream.read(buf.data(), size);

    if (buf[size - 1] != 0)
    {
        str.assign(buf.data(), size);
        if (str.size() != ((size_t)size)) {
            fail("getBZString string size mismatch");
        }
    }
    else
    {
        str.assign(buf.data(), size - 1); // don't copy null terminator
        if (str.size() != ((size_t)size - 1)) {
            fail("getBZString string size mismatch (null terminator)");
        }
    }
}

CompressedBSAFile::CompressedBSAFile()
    : mCompressedByDefault(false), mEmbeddedFileNames(false)
{ }

CompressedBSAFile::~CompressedBSAFile() = default;

/// Read header information from the input source
void CompressedBSAFile::readHeader()
{
    assert(!mIsLoaded);

    std::ifstream input(std::filesystem::path(mFilename), std::ios_base::binary);

    // Total archive size
    std::streamoff fsize = 0;
    if(input.seekg(0, std::ios_base::end))
    {
        fsize = input.tellg();
        input.seekg(0);
    }

    if(fsize < 36) // header is 36 bytes
        fail("File too small to be a valid BSA archive");

    // Get essential header numbers
    //size_t dirsize, filenum;
    std::uint32_t archiveFlags, folderCount, totalFileNameLength;
    {
        // First 36 bytes
        std::uint32_t header[9];

        input.read(reinterpret_cast<char*>(header), 36);

        if (header[0] != 0x00415342) /*"BSA\x00"*/
            fail("Unrecognized compressed BSA format");
        mVersion = header[1];
        if (mVersion != 0x67 /*TES4*/ && mVersion != 0x68 /*FO3, FNV, TES5*/ && mVersion != 0x69 /*SSE*/)
            fail("Unrecognized compressed BSA version");

        // header[2] is offset, should be 36 = 0x24 which is the size of the header

        // Oblivion - Meshes.bsa
        //
        // 0111 1000 0111 = 0x0787
        //  ^^^ ^     ^^^
        //  ||| |     ||+-- has names for dirs  (mandatory?)
        //  ||| |     |+--- has names for files (mandatory?)
        //  ||| |     +---- files are compressed by default
        //  ||| |
        //  ||| +---------- unknown (TES5: retain strings during startup)
        //  ||+------------ unknown (TES5: embedded file names)
        //  |+------------- unknown
        //  +-------------- unknown
        //
        archiveFlags          = header[3];
        folderCount           = header[4];
        // header[5] - fileCount
        // totalFolderNameLength = header[6];
        totalFileNameLength   = header[7];
        // header[8]; // fileFlags : an opportunity to optimize here

        mCompressedByDefault = (archiveFlags & 0x4) != 0;
        if (mVersion == 0x68 || mVersion == 0x69) /*FO3, FNV, TES5, SSE*/
            mEmbeddedFileNames = (archiveFlags & 0x100) != 0;
    }

    // folder records
    std::uint64_t hash;
    FolderRecord fr;
    for (std::uint32_t i = 0; i < folderCount; ++i)
    {
        input.read(reinterpret_cast<char*>(&hash), 8);
        input.read(reinterpret_cast<char*>(&fr.count), 4); // not sure purpose of count
        if (mVersion == 0x69) // SSE
        {
            std::uint32_t unknown;
            input.read(reinterpret_cast<char*>(&unknown), 4);
            input.read(reinterpret_cast<char*>(&fr.offset), 8);
        }
        else
            input.read(reinterpret_cast<char*>(&fr.offset), 4); // not sure purpose of offset

        auto lb = mFolders.lower_bound(hash);
        if (lb != mFolders.end() && !(mFolders.key_comp()(hash, lb->first)))
            fail("Archive found duplicate folder name hash");
        else
            mFolders.insert(lb, std::pair<std::uint64_t, FolderRecord>(hash, fr));
    }

    // file record blocks
    std::uint64_t fileHash;
    FileRecord file;

    std::string folder;
    std::uint64_t folderHash;
    if ((archiveFlags & 0x1) == 0)
        folderCount = 1; // TODO: not tested - unit test necessary

    mFiles.clear();
    std::vector<std::string> fullPaths;
    
    for (std::uint32_t i = 0; i < folderCount; ++i)
    {
        if ((archiveFlags & 0x1) != 0)
            getBZString(folder, input);

        folderHash = generateHash(folder, std::string());

        auto iter = mFolders.find(folderHash);
        if (iter == mFolders.end())
            fail("Archive folder name hash not found");

        for (std::uint32_t j = 0; j < iter->second.count; ++j)
        {
            input.read(reinterpret_cast<char*>(&fileHash), 8);
            input.read(reinterpret_cast<char*>(&file.size), 4);
            input.read(reinterpret_cast<char*>(&file.offset), 4);

            auto lb = iter->second.files.lower_bound(fileHash);
            if (lb != iter->second.files.end() && !(iter->second.files.key_comp()(fileHash, lb->first)))
                fail("Archive found duplicate file name hash");

            iter->second.files.insert(lb, std::pair<std::uint64_t, FileRecord>(fileHash, file));

            FileStruct fileStruct{};
            fileStruct.fileSize = file.getSizeWithoutCompressionFlag();
            fileStruct.offset = file.offset;
            mFiles.push_back(fileStruct);

            fullPaths.push_back(folder);
        }
    }

    // file record blocks
    if ((archiveFlags & 0x2) != 0)
    {
        mStringBuf.resize(totalFileNameLength);
        input.read(mStringBuf.data(), mStringBuf.size()); // TODO: maybe useful in building a lookup map?
    }

    size_t mStringBuffOffset = 0;
    size_t totalStringsSize = 0;
    for (std::uint32_t fileIndex = 0; fileIndex < mFiles.size(); ++fileIndex) {

        if (mStringBuffOffset >= totalFileNameLength) {
            fail("Corrupted names record in BSA file");
        }

        //The vector guarantees that its elements occupy contiguous memory
        mFiles[fileIndex].setNameInfos(mStringBuffOffset, &mStringBuf);

        fullPaths.at(fileIndex) += "\\" + std::string(mStringBuf.data() + mStringBuffOffset);

        while (mStringBuffOffset < totalFileNameLength) {
            if (mStringBuf[mStringBuffOffset] != '\0') {
                mStringBuffOffset++;
            }
            else {
                mStringBuffOffset++;
                break;
            }
        }
        //we want to keep one more 0 character at the end of each string
        totalStringsSize += fullPaths.at(fileIndex).length() + 1u;
    }
    mStringBuf.resize(totalStringsSize);

    mStringBuffOffset = 0;
    for (std::uint32_t fileIndex = 0u; fileIndex < mFiles.size(); fileIndex++) {
        size_t stringLength = fullPaths.at(fileIndex).length();

        std::copy(fullPaths.at(fileIndex).c_str(),
            //plus 1 because we also want to copy 0 at the end of the string
            fullPaths.at(fileIndex).c_str() + stringLength + 1u,
            mStringBuf.data() + mStringBuffOffset);

        mFiles[fileIndex].setNameInfos(mStringBuffOffset, &mStringBuf);

        mStringBuffOffset += stringLength + 1u;
    }

    if (mStringBuffOffset != mStringBuf.size()) {
        fail("Could not resolve names of files in BSA file");
    }

    convertCompressedSizesToUncompressed();
    mIsLoaded = true;
}

CompressedBSAFile::FileRecord CompressedBSAFile::getFileRecord(const std::string& str) const
{
    // Force-convert the path into something both Windows and UNIX can handle first
    // to make sure Boost doesn't think the entire path is the filename on Linux
    // and subsequently purge it to determine the file folder.
    std::string path = str;
    std::replace(path.begin(), path.end(), '\\', '/');

    std::filesystem::path p(path);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    
    std::string folder = p.parent_path().string();
    std::uint64_t folderHash = generateHash(folder, std::string());

    auto it = mFolders.find(folderHash);
    if (it == mFolders.end())
        return FileRecord(); // folder not found, return default which has offset of sInvalidOffset

    std::uint64_t fileHash = generateHash(stem, ext);
    auto iter = it->second.files.find(fileHash);
    if (iter == it->second.files.end())
        return FileRecord(); // file not found, return default which has offset of sInvalidOffset

    return iter->second;
}

Files::IStreamPtr CompressedBSAFile::getFile(const FileStruct* file) 
{
    FileRecord fileRec = getFileRecord(file->name());
    if (!fileRec.isValid()) {
        fail("File not found: " + std::string(file->name()));
    }
    return getFile(fileRec);
}

void CompressedBSAFile::addFile(const std::string& filename, std::istream& file)
{
    assert(false); //not implemented yet
    fail("Add file is not implemented for compressed BSA: " + filename);
}

Files::IStreamPtr CompressedBSAFile::getFile(const char* file)
{
    FileRecord fileRec = getFileRecord(file);
    if (!fileRec.isValid()) {
        fail("File not found: " + std::string(file));
    }
    return getFile(fileRec);
}

Files::IStreamPtr CompressedBSAFile::getFile(const FileRecord& fileRecord)
{
    size_t size = fileRecord.getSizeWithoutCompressionFlag();
    size_t uncompressedSize = size;
    bool compressed = fileRecord.isCompressed(mCompressedByDefault);
    Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilename, fileRecord.offset, size);
    std::istream* fileStream = streamPtr.get();
    if (mEmbeddedFileNames)
    {
        // Skip over the embedded file name
        char length = 0;
        fileStream->read(&length, 1);
        fileStream->ignore(length);
        size -= length + sizeof(char);
    }
    if (compressed)
    {
        fileStream->read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uint32_t));
        size -= sizeof(uint32_t);
    }
    auto memoryStreamPtr = std::make_unique<MemoryInputStream>(uncompressedSize);

    if (compressed)
    {
        if (mVersion != 0x69) // Non-SSE: zlib
        {
            boost::iostreams::filtering_streambuf<boost::iostreams::input> inputStreamBuf;
            inputStreamBuf.push(boost::iostreams::zlib_decompressor());
            inputStreamBuf.push(*fileStream);

            boost::iostreams::basic_array_sink<char> sr(memoryStreamPtr->getRawData(), uncompressedSize);
            boost::iostreams::copy(inputStreamBuf, sr);
        }
        else // SSE: lz4
        {
            auto buffer = std::vector<char>(size);
            fileStream->read(buffer.data(), size);
            LZ4F_decompressionContext_t context = nullptr;
            LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
            LZ4F_decompressOptions_t options = {};
            LZ4F_errorCode_t errorCode = LZ4F_decompress(context, memoryStreamPtr->getRawData(), &uncompressedSize, buffer.data(), &size, &options);
            if (LZ4F_isError(errorCode))
                fail("LZ4 decompression error (file " + mFilename + "): " + LZ4F_getErrorName(errorCode));
            errorCode = LZ4F_freeDecompressionContext(context);
            if (LZ4F_isError(errorCode))
                fail("LZ4 decompression error (file " + mFilename + "): " + LZ4F_getErrorName(errorCode));
        }
    }
    else
    {
        fileStream->read(memoryStreamPtr->getRawData(), size);
    }

    return std::make_unique<Files::StreamWithBuffer<MemoryInputStream>>(std::move(memoryStreamPtr));
}

BsaVersion CompressedBSAFile::detectVersion(const std::string& filePath)
{
    std::ifstream input(std::filesystem::path(filePath), std::ios_base::binary);

    // Total archive size
    std::streamoff fsize = 0;
    if (input.seekg(0, std::ios_base::end))
    {
        fsize = input.tellg();
        input.seekg(0);
    }

    if (fsize < 12) {
        return BSAVER_UNKNOWN;
    }

    // Get essential header numbers

    // First 12 bytes
    uint32_t head[3];

    input.read(reinterpret_cast<char*>(head), 12);

    if (head[0] == static_cast<uint32_t>(BSAVER_UNCOMPRESSED)) {
        return BSAVER_UNCOMPRESSED;
    }

    if (head[0] == static_cast<uint32_t>(BSAVER_COMPRESSED)) {
        return BSAVER_COMPRESSED;
    }

    return BSAVER_UNKNOWN;
}

//mFiles used by OpenMW expects uncompressed sizes
void CompressedBSAFile::convertCompressedSizesToUncompressed()
{
    for (auto & mFile : mFiles)
    {
        const FileRecord& fileRecord = getFileRecord(mFile.name());
        if (!fileRecord.isValid())
        {
            fail("Could not find file " + std::string(mFile.name()) + " in BSA");
        }

        if (!fileRecord.isCompressed(mCompressedByDefault))
        {
            //no need to fix fileSize in mFiles - uncompressed size already set
            continue;
        }

        Files::IStreamPtr dataBegin = Files::openConstrainedFileStream(mFilename, fileRecord.offset, fileRecord.getSizeWithoutCompressionFlag());

        if (mEmbeddedFileNames)
        {
            std::string embeddedFileName;
            getBZString(embeddedFileName, *(dataBegin.get()));
        }

        dataBegin->read(reinterpret_cast<char*>(&(mFile.fileSize)), sizeof(mFile.fileSize));
    }
}

std::uint64_t CompressedBSAFile::generateHash(std::string stem, std::string extension)
{
    size_t len = stem.length();
    if (len == 0)
        return 0;
    std::replace(stem.begin(), stem.end(), '/', '\\');
    Misc::StringUtils::lowerCaseInPlace(stem);
    uint64_t result = stem[len-1] | (len >= 3 ? (stem[len-2] << 8) : 0) | (len << 16) | (stem[0] << 24);
    if (len >= 4)
    {
        uint32_t hash = 0;
        for (size_t i = 1; i <= len-3; ++i)
            hash = hash * 0x1003f + stem[i];
        result += static_cast<uint64_t>(hash) << 32;
    }
    if (extension.empty())
        return result;
    Misc::StringUtils::lowerCaseInPlace(extension);
    if (extension == ".kf")       result |= 0x80;
    else if (extension == ".nif") result |= 0x8000;
    else if (extension == ".dds") result |= 0x8080;
    else if (extension == ".wav") result |= 0x80000000;
    uint32_t hash = 0;
    for (const char &c : extension)
        hash = hash * 0x1003f + c;
    result += static_cast<uint64_t>(hash) << 32;
    return result;
}

} //namespace Bsa
