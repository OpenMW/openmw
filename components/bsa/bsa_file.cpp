/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (bsa_file.cpp) is part of the OpenMW package.

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

#include "bsa_file.hpp"

#include <cassert>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace Bsa;


/// Error handling
void BSAFile::fail(const string &msg)
{
    throw std::runtime_error("BSA Error: " + msg + "\nArchive: " + mFilename);
}

/// Read header information from the input source
void BSAFile::readHeader()
{
    /*
     * The layout of a BSA archive is as follows:
     *
     * - 12 bytes header, contains 3 ints:
     *         id number - equal to 0x100
     *         dirsize - size of the directory block (see below)
     *         numfiles - number of files
     *
     * ---------- start of directory block -----------
     *
     * - 8 bytes*numfiles, each record contains:
     *         fileSize
     *         offset into data buffer (see below)
     *
     * - 4 bytes*numfiles, each record is an offset into the following name buffer
     *
     * - name buffer, indexed by the previous table, each string is
     *   null-terminated. Size is (dirsize - 12*numfiles).
     *
     * ---------- end of directory block -------------
     *
     * - 8*filenum - hash table block, we currently ignore this
     *
     * ----------- start of data buffer --------------
     *
     * - The rest of the archive is file data, indexed by the
     *   offsets in the directory block. The offsets start at 0 at
     *   the beginning of this buffer.
     *
     */
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

    if(fsize < 12)
        fail("File too small to be a valid BSA archive");

    // Get essential header numbers
    size_t dirsize, filenum;
    {
        // First 12 bytes
        uint32_t head[3];

        input.read(reinterpret_cast<char*>(head), 12);

        if(head[0] != 0x100)
            fail("Unrecognized BSA header");

        // Total number of bytes used in size/offset-table + filename
        // sections.
        dirsize = head[1];

        // Number of files
        filenum = head[2];
    }

    // Each file must take up at least 21 bytes of data in the bsa. So
    // if files*21 overflows the file size then we are guaranteed that
    // the archive is corrupt.
    if((filenum*21 > unsigned(fsize -12)) || (dirsize+8*filenum > unsigned(fsize -12)) )
        fail("Directory information larger than entire archive");

    // Read the offset info into a temporary buffer
    std::vector<uint32_t> offsets(3*filenum);
    input.read(reinterpret_cast<char*>(&offsets[0]), 12*filenum);

    // Read the string table
    mStringBuf.resize(dirsize-12*filenum);
    input.read(&mStringBuf[0], mStringBuf.size());

    // Check our position
    assert(input.tellg() == std::streampos(12+dirsize));

    // Calculate the offset of the data buffer. All file offsets are
    // relative to this. 12 header bytes + directory + hash table
    // (skipped)
    size_t fileDataOffset = 12 + dirsize + 8*filenum;

    // Set up the the FileStruct table
    mFiles.resize(filenum);
    for(size_t i=0;i<filenum;i++)
    {
        FileStruct &fs = mFiles[i];
        fs.fileSize = offsets[i*2];
        fs.offset = offsets[i*2+1] + fileDataOffset;
        fs.name = &mStringBuf[offsets[2*filenum+i]];

        if(fs.offset + fs.fileSize > fsize)
            fail("Archive contains offsets outside itself");

        // Add the file name to the lookup
        mLookup[fs.name] = i;
    }

    mIsLoaded = true;
}

/// Get the index of a given file name, or -1 if not found
int BSAFile::getIndex(const char *str) const
{
    Lookup::const_iterator it = mLookup.find(str);
    if(it == mLookup.end())
        return -1;

    int res = it->second;
    assert(res >= 0 && (size_t)res < mFiles.size());
    return res;
}

/// Open an archive file.
void BSAFile::open(const string &file)
{
    mFilename = file;
    readHeader();
}

Files::IStreamPtr BSAFile::getFile(const char *file)
{
    assert(file);
    int i = getIndex(file);
    if(i == -1)
        fail("File not found: " + string(file));

    const FileStruct &fs = mFiles[i];

    return Files::openConstrainedFileStream (mFilename.c_str (), fs.offset, fs.fileSize);
}

Files::IStreamPtr BSAFile::getFile(const FileStruct *file)
{
    return Files::openConstrainedFileStream (mFilename.c_str (), file->offset, file->fileSize);
}
