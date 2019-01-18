/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (bsa_file.h) is part of the OpenMW package.

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

  TES4 stuff added by cc9cii 2018

 */

#ifndef BSA_TES4BSA_FILE_H
#define BSA_TES4BSA_FILE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include <OgreDataStream.h>


namespace Bsa
{
    class TES4BSAFile
    {
    public:
        struct FileRecord
        {
            std::uint32_t size;
            std::uint32_t offset;

            std::string fileName; // NOTE: for testing hash collision only, see TEST_UNIQUE_HASH

            FileRecord() : size(0), offset(-1) {}
        };

    private:
        /// Filenames string buffer
        std::vector<char> mStringBuf;

        /// True when an archive has been loaded
        bool isLoaded;

        bool mCompressedByDefault;
        bool mEmbeddedFileNames;

        std::map<std::uint64_t, FileRecord> mFiles;
        typedef std::map<std::uint64_t, FileRecord> FileList;

        struct FolderRecord
        {
            std::uint32_t count;
            std::uint32_t offset;
            std::map<std::uint64_t, FileRecord> files;
        };
        std::map<std::uint64_t, FolderRecord> mFolders;

        FileRecord getFileRecord(const std::string& str) const;

        /// Used for error messages and getting files
        std::string mFilename;

        /// Error handling
        void fail(const std::string &msg);

        /// Read header information from the input source
        void readHeader();

    public:
        TES4BSAFile()
          : isLoaded(false), mCompressedByDefault(false), mEmbeddedFileNames(false)
        { }

        /// Open an archive file.
        void open(const std::string &file);

        /// Check if a file exists
        bool exists(const std::string& file) const;

        Ogre::DataStreamPtr getFile(const std::string& file);

        /// Get a list of all files
        const FileList &getList() const // FIXME
        { return mFiles; }
    };
}

#endif
