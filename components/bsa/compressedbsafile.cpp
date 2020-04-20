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

#include <boost/scoped_array.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <components/bsa/memorystream.hpp>

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

    boost::scoped_array<char> buf(new char[size]);
    filestream.read(buf.get(), size);

    if (buf[size - 1] != 0)
    {
        str.assign(buf.get(), size);
        if (str.size() != ((size_t)size)) {
            fail("getBZString string size mismatch");
        }
    }
    else
    {
        str.assign(buf.get(), size - 1); // don't copy null terminator
        if (str.size() != ((size_t)size - 1)) {
            fail("getBZString string size mismatch (null terminator)");
        }
    }
}

CompressedBSAFile::CompressedBSAFile()
    : mCompressedByDefault(false), mEmbeddedFileNames(false)
{ }

CompressedBSAFile::~CompressedBSAFile()
{ }

/// Read header information from the input source
void CompressedBSAFile::readHeader()
{
    assert(!mIsLoaded);

    namespace bfs = boost::filesystem;
    bfs::ifstream input(bfs::path(mFilename), std::ios_base::binary);

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

        if(header[0] != 0x00415342 /*"BSA\x00"*/ || (header[1] != 0x67 /*TES4*/ && header[1] != 0x68 /*TES5*/))
            fail("Unrecognized TES4 BSA header");

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
        mEmbeddedFileNames = header[1] == 0x68 /*TES5*/ && (archiveFlags & 0x100) != 0;
    }

    // folder records
    std::uint64_t hash;
    FolderRecord fr;
    for (std::uint32_t i = 0; i < folderCount; ++i)
    {
        input.read(reinterpret_cast<char*>(&hash), 8);
        input.read(reinterpret_cast<char*>(&fr.count), 4); // not sure purpose of count
        input.read(reinterpret_cast<char*>(&fr.offset), 4); // not sure purpose of offset

        std::map<std::uint64_t, FolderRecord>::const_iterator lb = mFolders.lower_bound(hash);
        if (lb != mFolders.end() && !(mFolders.key_comp()(hash, lb->first)))
            fail("Archive found duplicate folder name hash");
        else
            mFolders.insert(lb, std::pair<std::uint64_t, FolderRecord>(hash, fr));
    }

    // file record blocks
    std::uint64_t fileHash;
    FileRecord file;

    std::string folder("");
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

        std::map<std::uint64_t, FolderRecord>::iterator iter = mFolders.find(folderHash);
        if (iter == mFolders.end())
            fail("Archive folder name hash not found");

        for (std::uint32_t j = 0; j < iter->second.count; ++j)
        {
            input.read(reinterpret_cast<char*>(&fileHash), 8);
            input.read(reinterpret_cast<char*>(&file.size), 4);
            input.read(reinterpret_cast<char*>(&file.offset), 4);

            std::map<std::uint64_t, FileRecord>::const_iterator lb = iter->second.files.lower_bound(fileHash);
            if (lb != iter->second.files.end() && !(iter->second.files.key_comp()(fileHash, lb->first)))
                fail("Archive found duplicate file name hash");

            iter->second.files.insert(lb, std::pair<std::uint64_t, FileRecord>(fileHash, file));

            FileStruct fileStruct;
            fileStruct.fileSize = file.getSizeWithoutCompressionFlag();
            fileStruct.offset = file.offset;
            fileStruct.name = nullptr;
            mFiles.push_back(fileStruct);

            fullPaths.push_back(folder);
        }
    }

    // file record blocks
    if ((archiveFlags & 0x2) != 0)
    {
        mStringBuf.resize(totalFileNameLength);
        input.read(&mStringBuf[0], mStringBuf.size()); // TODO: maybe useful in building a lookup map?
    }

    size_t mStringBuffOffset = 0;
    size_t totalStringsSize = 0;
    for (std::uint32_t fileIndex = 0; fileIndex < mFiles.size(); ++fileIndex) {

        if (mStringBuffOffset >= totalFileNameLength) {
            fail("Corrupted names record in BSA file");
        }

        //The vector guarantees that its elements occupy contiguous memory
        mFiles[fileIndex].name = reinterpret_cast<char*>(mStringBuf.data() + mStringBuffOffset);

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

        mFiles[fileIndex].name = reinterpret_cast<char*>(mStringBuf.data() + mStringBuffOffset);

        mLookup[reinterpret_cast<char*>(mStringBuf.data() + mStringBuffOffset)] = fileIndex;
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

    boost::filesystem::path p(path);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    p.remove_filename();

    std::string folder = p.string();
    std::uint64_t folderHash = generateHash(folder, std::string());

    std::map<std::uint64_t, FolderRecord>::const_iterator it = mFolders.find(folderHash);
    if (it == mFolders.end())
        return FileRecord(); // folder not found, return default which has offset of sInvalidOffset

    std::uint64_t fileHash = generateHash(stem, ext);
    std::map<std::uint64_t, FileRecord>::const_iterator iter = it->second.files.find(fileHash);
    if (iter == it->second.files.end())
        return FileRecord(); // file not found, return default which has offset of sInvalidOffset

    return iter->second;
}

Files::IStreamPtr CompressedBSAFile::getFile(const FileStruct* file) 
{
    FileRecord fileRec = getFileRecord(file->name);
    if (!fileRec.isValid()) {
        fail("File not found: " + std::string(file->name));
    }
    return getFile(fileRec);
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
    if (fileRecord.isCompressed(mCompressedByDefault)) {
        Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilename.c_str(), fileRecord.offset, fileRecord.getSizeWithoutCompressionFlag());

        std::istream* fileStream = streamPtr.get();

        if (mEmbeddedFileNames) {
            std::string embeddedFileName;
            getBZString(embeddedFileName, *fileStream);
        }

        uint32_t uncompressedSize = 0u;
        fileStream->read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));

        boost::iostreams::filtering_streambuf<boost::iostreams::input> inputStreamBuf;
        inputStreamBuf.push(boost::iostreams::zlib_decompressor());
        inputStreamBuf.push(*fileStream);

        std::shared_ptr<Bsa::MemoryInputStream> memoryStreamPtr = std::make_shared<MemoryInputStream>(uncompressedSize);

        boost::iostreams::basic_array_sink<char> sr(memoryStreamPtr->getRawData(), uncompressedSize);
        boost::iostreams::copy(inputStreamBuf, sr);

        return std::shared_ptr<std::istream>(memoryStreamPtr, (std::istream*)memoryStreamPtr.get());
    }

    return Files::openConstrainedFileStream(mFilename.c_str(), fileRecord.offset, fileRecord.size);
}

BsaVersion CompressedBSAFile::detectVersion(std::string filePath)
{
    namespace bfs = boost::filesystem;
    bfs::ifstream input(bfs::path(filePath), std::ios_base::binary);

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
    for (auto iter = mFiles.begin(); iter != mFiles.end(); ++iter)
    {
        const FileRecord& fileRecord = getFileRecord(iter->name);
        if (!fileRecord.isValid())
        {
            fail("Could not find file " + std::string(iter->name) + " in BSA");
        }

        if (!fileRecord.isCompressed(mCompressedByDefault))
        {
            //no need to fix fileSize in mFiles - uncompressed size already set
            continue;
        }

        Files::IStreamPtr dataBegin = Files::openConstrainedFileStream(mFilename.c_str(), fileRecord.offset, fileRecord.getSizeWithoutCompressionFlag());

        if (mEmbeddedFileNames)
        {
            std::string embeddedFileName;
            getBZString(embeddedFileName, *(dataBegin.get()));
        }

        dataBegin->read(reinterpret_cast<char*>(&(iter->fileSize)), sizeof(iter->fileSize));
    }
}

std::uint64_t CompressedBSAFile::generateHash(std::string stem, std::string extension) const
{
    size_t len = stem.length();
    if (len == 0)
        return 0;
    std::uint64_t hash = 0;
    unsigned int hash2 = 0;
    Misc::StringUtils::lowerCaseInPlace(stem);
    if (extension.empty()) // It's a folder.
        std::replace(stem.begin(), stem.end(), '/', '\\');
    else
    {
        Misc::StringUtils::lowerCaseInPlace(extension);
        for (const char &c : extension)
            hash = hash * 0x1003f + c;
    }
    if (len >= 4)
    {
        for (size_t i = 1; i < len-2; i++)
            hash2 = hash2 * 0x1003f + stem[i];
    }
    hash = (hash + hash2) << 32;
    hash2 = (stem[0] << 24) | (len << 16);
    if (len >= 2)
    {
        if (len >= 3)
            hash2 |= stem[len-2] << 8;
        hash2 |= stem[len-1];
    }
    if (!extension.empty())
    {
        if (extension == ".kf")       hash2 |= 0x80;
        else if (extension == ".nif") hash2 |= 0x8000;
        else if (extension == ".dds") hash2 |= 0x8080;
        else if (extension == ".wav") hash2 |= 0x80000000;
    }
    return hash + hash2;
}

} //namespace Bsa
