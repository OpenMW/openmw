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

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <filesystem>
#include <format>
#include <istream>
#include <system_error>

#include <lz4frame.h>
#include <zlib.h>

#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>
#include <components/files/utils.hpp>
#include <components/misc/strings/lower.hpp>

#include "memorystream.hpp"

namespace Bsa
{
    /// Read header information from the input source
    void CompressedBSAFile::readHeader(std::istream& input)
    {
        assert(!mIsLoaded);

        const std::streamsize fsize = Files::getStreamSizeLeft(input);

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
        if (input.fail())
            fail("Failed to read compressed BSA folder record offset: " + std::generic_category().message(errno));

        struct FlatFolderRecord
        {
            std::uint64_t mHash;
            std::string mName;
            std::uint32_t mCount;
            std::int64_t mOffset;
        };

        std::vector<std::pair<FlatFolderRecord, std::vector<FileRecord>>> folders;
        folders.reserve(mHeader.mFolderCount);

        for (std::uint32_t i = 0; i < mHeader.mFolderCount; ++i)
        {
            FlatFolderRecord folder;

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

            if (input.fail())
                fail(std::format(
                    "Failed to read compressed BSA folder record: {}", std::generic_category().message(errno)));

            folders.emplace_back(std::move(folder), std::vector<FileRecord>());
        }

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

            filelist.reserve(folder.mCount);

            for (std::uint32_t i = 0; i < folder.mCount; ++i)
            {
                FileRecord file;

                input.read(reinterpret_cast<char*>(&file.mHash), 8);
                input.read(reinterpret_cast<char*>(&file.mSize), 4);
                input.read(reinterpret_cast<char*>(&file.mOffset), 4);

                if (input.fail())
                    fail(std::format("Failed to read compressed BSA folder file record: {}",
                        std::generic_category().message(errno)));

                filelist.push_back(std::move(file));
            }
        }

        if (mHeader.mFolderNamesLength != 0)
            input.ignore(mHeader.mFolderNamesLength);

        if (input.fail())
            fail(std::format("Failed to read compressed BSA file records: {}", std::generic_category().message(errno)));

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

        if (input.fail())
            fail(std::format("Failed to read compressed BSA filenames: {}", std::generic_category().message(errno)));

        for (auto& [folder, filelist] : folders)
        {
            std::map<std::uint64_t, FileRecord> fileMap;

            for (auto& file : filelist)
                fileMap[file.mHash] = std::move(file);

            mFolders[folder.mHash] = FolderRecord{ folder.mCount, folder.mOffset, folder.mName, std::move(fileMap) };
        }

        for (auto& [folderHash, folderRecord] : mFolders)
        {
            for (auto& [fileHash, fileRecord] : folderRecord.mFiles)
            {
                FileStruct fileStruct{};
                fileStruct.mFileSize = fileRecord.mSize & (~FileSizeFlag_Compression);
                fileStruct.mOffset = fileRecord.mOffset;
                fileStruct.mNameOffset = 0;
                fileStruct.mNameSize
                    = fileRecord.mName.empty() ? 0 : static_cast<uint32_t>(fileRecord.mName.size() - 1);
                fileStruct.mNamesBuffer = &fileRecord.mName;
                mFiles.push_back(fileStruct);
            }
        }
    }

    CompressedBSAFile::FileRecord CompressedBSAFile::getFileRecord(std::string_view str) const
    {
        for (const auto c : str)
        {
            if (((static_cast<unsigned>(c) >> 7U) & 1U) != 0U)
            {
                fail(std::format("File record {} contains unicode characters, refusing to load.", str));
            }
        }

#ifdef _WIN32
        const auto& path = str;
#else
        // Force-convert the path into something UNIX can handle first
        // to make sure std::filesystem::path doesn't think the entire path is the filename on Linux
        // and subsequently purge it to determine the file folder.
        std::string path(str);
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
            std::vector<char> buffer(size);
            streamPtr->read(buffer.data(), size);

            if (mHeader.mVersion != Version_SSE)
            {
                uLongf destSize = static_cast<uLongf>(resultSize);
                int ec = ::uncompress(reinterpret_cast<Bytef*>(memoryStreamPtr->getRawData()), &destSize,
                    reinterpret_cast<Bytef*>(buffer.data()), static_cast<uLong>(buffer.size()));

                if (ec != Z_OK)
                {
                    std::string message = "zlib uncompress failed for file ";
                    message.append(fileRecord.mName.begin(), fileRecord.mName.end());
                    message += ": ";
                    message += ::zError(ec);
                    fail(message);
                }
            }
            else
            {
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
