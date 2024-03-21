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

#include <cassert>
#include <filesystem>
#include <fstream>

#include <lz4frame.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4706)
#pragma warning(disable : 4702)
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#pragma warning(pop)
#else
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#endif

#include <boost/iostreams/device/array.hpp>
#include <components/bsa/memorystream.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/lower.hpp>

namespace Bsa
{
    /// Read header information from the input source
    void CompressedBSAFile::readHeader()
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

        if (fsize < 36) // Header is 36 bytes
            fail("File too small to be a valid BSA archive");

        input.read(reinterpret_cast<char*>(&mHeader), sizeof(mHeader));

        if (mHeader.mFormat != static_cast<std::uint32_t>(BsaVersion::Compressed)) // BSA
            fail("Unrecognized compressed BSA format");
        if (mHeader.mVersion != Version_TES4 && mHeader.mVersion != Version_FO3 && mHeader.mVersion != Version_SSE)
            fail("Unrecognized compressed BSA version");

        if (mHeader.mVersion == Version_TES4)
            mHeader.mFlags &= (~ArchiveFlag_EmbeddedNames);

        input.seekg(mHeader.mFoldersOffset);
        if (input.bad())
            fail("Invalid compressed BSA folder record offset");

        struct FlatFolderRecord
        {
            std::uint64_t mHash;
            std::string mName;
            std::uint32_t mCount;
            std::int64_t mOffset;
        };

        std::vector<std::pair<FlatFolderRecord, std::vector<FileRecord>>> folders;
        folders.resize(mHeader.mFolderCount);
        for (auto& [folder, filelist] : folders)
        {
            input.read(reinterpret_cast<char*>(&folder.mHash), 8);
            input.read(reinterpret_cast<char*>(&folder.mCount), 4);
            if (mHeader.mVersion == Version_SSE) // SSE
            {
                std::uint32_t unknown = 0;
                input.read(reinterpret_cast<char*>(&unknown), 4);
                input.read(reinterpret_cast<char*>(&folder.mOffset), 8);
            }
            else
            {
                input.read(reinterpret_cast<char*>(&folder.mOffset), 4);
            }
        }

        if (input.bad())
            fail("Failed to read compressed BSA folder records: input error");

        // file record blocks
        if ((mHeader.mFlags & ArchiveFlag_FolderNames) == 0)
            mHeader.mFolderCount = 1; // TODO: not tested - unit test necessary

        for (auto& [folder, filelist] : folders)
        {
            if ((mHeader.mFlags & ArchiveFlag_FolderNames) != 0)
            {
                uint8_t size = 0;
                input.read(reinterpret_cast<char*>(&size), 1);
                if (size > 0)
                {
                    folder.mName.resize(size);
                    input.getline(folder.mName.data(), size, '\0');
                    if (input.gcount() != static_cast<std::streamsize>(size))
                        fail("Folder name size mismatch: " + std::to_string(input.gcount()) + " bytes read, expected "
                            + std::to_string(size));
                    if (mHeader.mFolderNamesLength < size)
                        fail("Failed to read folder names: " + std::to_string(mHeader.mFolderNamesLength) + " < "
                            + std::to_string(size));
                    folder.mName.resize(size - 1);
                }
                mHeader.mFolderNamesLength -= size;
            }

            filelist.resize(folder.mCount);
            for (auto& file : filelist)
            {
                input.read(reinterpret_cast<char*>(&file.mHash), 8);
                input.read(reinterpret_cast<char*>(&file.mSize), 4);
                input.read(reinterpret_cast<char*>(&file.mOffset), 4);
            }
        }

        if (mHeader.mFolderNamesLength != 0)
            input.ignore(mHeader.mFolderNamesLength);

        if (input.bad())
            fail("Failed to read compressed BSA file records: input error");

        if ((mHeader.mFlags & ArchiveFlag_FileNames) != 0)
        {
            for (auto& [folder, filelist] : folders)
            {
                for (auto& file : filelist)
                {
                    auto& name = file.mName;
                    name.resize(256);
                    input.getline(name.data(), 256, '\0');
                    if (input.gcount() <= 1)
                        fail("Failed to read a filename: filename is empty");
                    if (mHeader.mFileNamesLength < input.gcount())
                        fail("Failed to read file names: " + std::to_string(mHeader.mFileNamesLength) + " < "
                            + std::to_string(input.gcount()));
                    name.resize(input.gcount());
                    if (name.back() != '\0')
                        fail("Failed to read a filename: filename is too long");
                    mHeader.mFileNamesLength -= static_cast<std::uint32_t>(input.gcount());
                    file.mName.insert(file.mName.begin(), folder.mName.begin(), folder.mName.end());
                    file.mName.insert(file.mName.begin() + folder.mName.size(), '\\');
                }
            }
        }

        if (mHeader.mFileNamesLength != 0)
            input.ignore(mHeader.mFileNamesLength);

        if (input.bad())
            fail("Failed to read compressed BSA filenames: input error");

        for (auto& [folder, filelist] : folders)
        {
            std::map<std::uint64_t, FileRecord> fileMap;
            for (const auto& file : filelist)
                fileMap[file.mHash] = std::move(file);
            auto& folderMap = mFolders[folder.mHash];
            folderMap = FolderRecord{ folder.mCount, folder.mOffset, std::move(fileMap) };
            for (auto& [hash, fileRec] : folderMap.mFiles)
            {
                FileStruct fileStruct{};
                fileStruct.fileSize = fileRec.mSize & (~FileSizeFlag_Compression);
                fileStruct.offset = fileRec.mOffset;
                fileStruct.setNameInfos(0, &fileRec.mName);
                mFiles.emplace_back(fileStruct);
            }
        }

        mIsLoaded = true;
    }

    CompressedBSAFile::FileRecord CompressedBSAFile::getFileRecord(const std::string& str) const
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
        const auto stem = p.stem();
        const auto ext = p.extension().string(); // Purposefully damage Unicode strings.

        std::uint64_t folderHash = generateHash(p.parent_path(), {});

        auto it = mFolders.find(folderHash);
        if (it == mFolders.end())
            return FileRecord();

        std::uint64_t fileHash = generateHash(stem, ext);
        auto iter = it->second.mFiles.find(fileHash);
        if (iter == it->second.mFiles.end())
            return FileRecord();

        return iter->second;
    }

    Files::IStreamPtr CompressedBSAFile::getFile(const FileStruct* file)
    {
        FileRecord fileRec = getFileRecord(file->name());
        if (fileRec.mOffset == std::numeric_limits<uint32_t>::max())
        {
            fail("File not found: " + std::string(file->name()));
        }
        return getFile(fileRec);
    }

    void CompressedBSAFile::addFile(const std::string& filename, std::istream& file)
    {
        assert(false); // not implemented yet
        fail("Add file is not implemented for compressed BSA: " + filename);
    }

    Files::IStreamPtr CompressedBSAFile::getFile(const char* file)
    {
        FileRecord fileRec = getFileRecord(file);
        if (fileRec.mOffset == std::numeric_limits<uint32_t>::max())
        {
            fail("File not found: " + std::string(file));
        }
        return getFile(fileRec);
    }

    Files::IStreamPtr CompressedBSAFile::getFile(const FileRecord& fileRecord)
    {
        size_t size = fileRecord.mSize & (~FileSizeFlag_Compression);
        size_t resultSize = size;
        Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilepath, fileRecord.mOffset, size);
        bool compressed = (fileRecord.mSize != size) == ((mHeader.mFlags & ArchiveFlag_Compress) == 0);
        if ((mHeader.mFlags & ArchiveFlag_EmbeddedNames) != 0)
        {
            // Skip over the embedded file name
            uint8_t length = 0;
            streamPtr->read(reinterpret_cast<char*>(&length), 1);
            streamPtr->ignore(length);
            size -= length + sizeof(uint8_t);
        }
        if (compressed)
        {
            streamPtr->read(reinterpret_cast<char*>(&resultSize), sizeof(uint32_t));
            size -= sizeof(uint32_t);
        }
        auto memoryStreamPtr = std::make_unique<MemoryInputStream>(resultSize);

        if (compressed)
        {
            if (mHeader.mVersion != Version_SSE)
            {
                boost::iostreams::filtering_streambuf<boost::iostreams::input> inputStreamBuf;
                inputStreamBuf.push(boost::iostreams::zlib_decompressor());
                inputStreamBuf.push(*streamPtr);

                boost::iostreams::basic_array_sink<char> sr(memoryStreamPtr->getRawData(), resultSize);
                boost::iostreams::copy(inputStreamBuf, sr);
            }
            else
            {
                auto buffer = std::vector<char>(size);
                streamPtr->read(buffer.data(), size);
                LZ4F_decompressionContext_t context = nullptr;
                LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
                LZ4F_decompressOptions_t options = {};
                LZ4F_errorCode_t errorCode = LZ4F_decompress(
                    context, memoryStreamPtr->getRawData(), &resultSize, buffer.data(), &size, &options);
                if (LZ4F_isError(errorCode))
                    fail("LZ4 decompression error (file " + Files::pathToUnicodeString(mFilepath)
                        + "): " + LZ4F_getErrorName(errorCode));
                errorCode = LZ4F_freeDecompressionContext(context);
                if (LZ4F_isError(errorCode))
                    fail("LZ4 decompression error (file " + Files::pathToUnicodeString(mFilepath)
                        + "): " + LZ4F_getErrorName(errorCode));
            }
        }
        else
        {
            streamPtr->read(memoryStreamPtr->getRawData(), size);
        }

        return std::make_unique<Files::StreamWithBuffer<MemoryInputStream>>(std::move(memoryStreamPtr));
    }

    std::uint64_t CompressedBSAFile::generateHash(const std::filesystem::path& stem, std::string extension)
    {
        auto str = stem.u8string();
        size_t len = str.length();
        if (len == 0)
            return 0;
        std::replace(str.begin(), str.end(), '/', '\\');
        Misc::StringUtils::lowerCaseInPlace(str);
        uint64_t result = str[len - 1];
        if (len >= 3)
            result |= str[len - 2] << 8;
        result |= len << 16;
        result |= static_cast<uint32_t>(str[0] << 24);
        if (len >= 4)
        {
            uint32_t hash = 0;
            for (size_t i = 1; i <= len - 3; ++i)
                hash = hash * 0x1003f + str[i];
            result += static_cast<uint64_t>(hash) << 32;
        }
        if (extension.empty())
            return result;
        Misc::StringUtils::lowerCaseInPlace(extension);
        if (extension == ".kf")
            result |= 0x80;
        else if (extension == ".nif")
            result |= 0x8000;
        else if (extension == ".dds")
            result |= 0x8080;
        else if (extension == ".wav")
            result |= 0x80000000;
        uint32_t hash = 0;
        for (const auto& c : extension)
            hash = hash * 0x1003f + c;
        result += static_cast<uint64_t>(hash) << 32;
        return result;
    }

} // namespace Bsa
