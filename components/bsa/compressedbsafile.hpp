/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (compressedbsafile.hpp) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_BSA_COMPRESSEDBSAFILE_HPP
#define OPENMW_COMPONENTS_BSA_COMPRESSEDBSAFILE_HPP

#include <filesystem>
#include <limits>
#include <map>

#include "bsafile.hpp"

namespace Bsa
{
    class CompressedBSAFile : private BSAFile
    {
    private:
        enum ArchiveFlags
        {
            ArchiveFlag_FolderNames = 0x0001,
            ArchiveFlag_FileNames = 0x0002,
            ArchiveFlag_Compress = 0x0004,
            ArchiveFlag_RetainDir = 0x0008,
            ArchiveFlag_RetainName = 0x0010,
            ArchiveFlag_RetainFileOffset = 0x0020,
            ArchiveFlag_Xbox360 = 0x0040,
            ArchiveFlag_StartUpStr = 0x0080,
            ArchiveFlag_EmbeddedNames = 0x0100,
            ArchiveFlag_XMem = 0x0200,
        };

        enum Version
        {
            Version_TES4 = 0x67,
            Version_FO3 = 0x68,
            Version_SSE = 0x69,
        };

        enum FileFlags
        {
            FileFlag_NIF = 0x001,
            FileFlag_DDS = 0x002,
            FileFlag_XML = 0x004,
            FileFlag_WAV = 0x008,
            FileFlag_MP3 = 0x010,
            FileFlag_TXT = 0x020, // TXT, HTML, BAT, SCC
            FileFlag_SPT = 0x040,
            FileFlag_FNT = 0x080, // TEX, FNT
            FileFlag_MISC = 0x100, // CTL and others
        };

        enum FileSizeFlags
        {
            FileSizeFlag_Compression = 0x40000000,
        };

        struct Header
        {
            std::uint32_t mFormat;
            std::uint32_t mVersion;
            std::uint32_t mFoldersOffset;
            std::uint32_t mFlags;
            std::uint32_t mFolderCount;
            std::uint32_t mFileCount;
            std::uint32_t mFolderNamesLength;
            std::uint32_t mFileNamesLength;
            std::uint32_t mFileFlags;
        };

        Header mHeader;

        struct FileRecord
        {
            std::uint64_t mHash;
            std::vector<char> mName;
            std::uint32_t mSize{ 0u };
            std::uint32_t mOffset{ std::numeric_limits<uint32_t>::max() };
        };

        struct FolderRecord
        {
            std::uint32_t mCount;
            std::int64_t mOffset;
            std::map<std::uint64_t, FileRecord> mFiles;
        };

        std::map<std::uint64_t, FolderRecord> mFolders;

        FileRecord getFileRecord(const std::string& str) const;

        /// \brief Normalizes given filename or folder and generates format-compatible hash.
        static std::uint64_t generateHash(const std::filesystem::path& stem, std::string extension);
        Files::IStreamPtr getFile(const FileRecord& fileRecord);

    public:
        using BSAFile::getFilename;
        using BSAFile::getList;
        using BSAFile::open;

        CompressedBSAFile() = default;
        virtual ~CompressedBSAFile() = default;

        /// Read header information from the input source
        void readHeader() override;

        Files::IStreamPtr getFile(const char* filePath);
        Files::IStreamPtr getFile(const FileStruct* fileStruct);
        void addFile(const std::string& filename, std::istream& file);
    };
}

#endif
