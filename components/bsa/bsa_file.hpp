/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

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
  https://www.gnu.org/licenses/ .

 */

#ifndef BSA_BSA_FILE_H
#define BSA_BSA_FILE_H

#include <cstdint>
#include <string>
#include <vector>

#include <components/files/constrainedfilestream.hpp>


namespace Bsa
{

/**
   This class is used to read "Bethesda Archive Files", or BSAs.
 */
class BSAFile
{
public:

    #pragma pack(push)
    #pragma pack(1)
    struct Hash
    {
        uint32_t low, high;
    };
    #pragma pack(pop)

    /// Represents one file entry in the archive
    struct FileStruct
    {
        void setNameInfos(size_t index,
            std::vector<char>* stringBuf
        ) {
            namesOffset = static_cast<uint32_t>(index);
            namesBuffer = stringBuf;
        }

        // File size and offset in file. We store the offset from the
        // beginning of the file, not the offset into the data buffer
        // (which is what is stored in the archive.)
        uint32_t fileSize, offset;
        Hash hash;

        // Zero-terminated file name
        const char* name() const { return &(*namesBuffer)[namesOffset]; };

        uint32_t namesOffset = 0;
        std::vector<char>* namesBuffer = nullptr;
    };
    typedef std::vector<FileStruct> FileList;

protected:
    bool mHasChanged = false;

    /// Table of files in this archive
    FileList mFiles;

    /// Filename string buffer
    std::vector<char> mStringBuf;

    /// True when an archive has been loaded
    bool mIsLoaded;

    /// Used for error messages
    std::string mFilename;

    /// Error handling
    [[noreturn]] void fail(const std::string &msg);

    /// Read header information from the input source
    virtual void readHeader();
    virtual void writeHeader();

public:
    /* -----------------------------------
     * BSA management methods
     * -----------------------------------
     */

    BSAFile()
      : mIsLoaded(false)
    { }

    virtual ~BSAFile()
    {
        close();
    }

    /// Open an archive file.
    void open(const std::string &file);

    void close();

    /* -----------------------------------
     * Archive file routines
     * -----------------------------------
     */

    /** Open a file contained in the archive.
     * @note Thread safe.
    */
    Files::IStreamPtr getFile(const FileStruct *file)
    {
        return Files::openConstrainedFileStream(mFilename, file->offset, file->fileSize);
    }

    void addFile(const std::string& filename, std::istream& file);

    /// Get a list of all files
    /// @note Thread safe.
    const FileList &getList() const
    { return mFiles; }

    const std::string& getFilename() const
    {
        return mFilename;
    }
};

}

#endif
