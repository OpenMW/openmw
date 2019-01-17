/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (tes4bsa_file.hpp) is part of the OpenMW package.

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
#include <components/bsa/bsa_file.hpp>

namespace Bsa
{
    enum BsaVersion
    {
        BSAVER_UNKNOWN = 0x0,
        BSAVER_TES3 = 0x100,
        BSAVER_TES4PLUS = 0x415342 //B, S, A
    };

/**
    This class is used to read "Bethesda Archive Files", or BSAs 
    in newer formats first introduced in TES IV Oblivion
 */
class TES4BSAFile : public BSAFile
{
public:
    //checks version of BSA from file header
    static BsaVersion detectVersion(std::string filePath);

private:
    //special marker for invalid records,
    //equal to max uint32_t value
    static const uint32_t sInvalidOffset;

    //bit marking compression on file size
    static const uint32_t sCompressedFlag;

    struct FileRecord
    {
        //size of file, with 30th bit containing compression flag
        std::uint32_t size;
        std::uint32_t offset;
        
        FileRecord();
        bool isCompressed(bool bsaCompressedByDefault) const;
        bool isValid() const;
        std::uint32_t getSizeWithoutCompressionFlag() const;
    };
  
    //if files in BSA  without 30th bit enabled are compressed
    bool mCompressedByDefault;

    //if file names are not present in file header,
    //and we need to look for them at given file offset
    bool mEmbeddedFileNames;

    struct FolderRecord
    {
        std::uint32_t count;
        std::uint32_t offset;
        std::map<std::uint64_t, FileRecord> files;
    };
    std::map<std::uint64_t, FolderRecord> mFolders;

    FileRecord getFileRecord(const std::string& filePath) const;

    /// Read header information from the input source
    virtual void readHeader();
public:
    TES4BSAFile();
    virtual ~TES4BSAFile();

    virtual Files::IStreamPtr getFile(const char* file);
    virtual Files::IStreamPtr getFile(const FileStruct* file);
 
private:
    Files::IStreamPtr getFile(const FileRecord& file);
    //mFiles used by OpenMW expects uncompressed sizes
    void convertCompressedSizesToUncompressed();
    void getBZString(std::string& str, std::istream& filestream);
};
}

#endif
