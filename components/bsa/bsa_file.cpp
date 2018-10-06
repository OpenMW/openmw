/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

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
  http://www.gnu.org/licenses/ .

 */

#include "bsa_file.hpp"

#include <stdexcept>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

#include "../files/constrainedfiledatastream.hpp"

namespace
{
    // see: http://en.uesp.net/wiki/Tes3Mod:BSA_File_Format
    std::uint64_t getHash(const char *name)
    {
        unsigned int len = (unsigned int)strlen(name);
        std::uint64_t hash;

        unsigned l = (len>>1);
        unsigned sum, off, temp, i, n, hash1;

        for(sum = off = i = 0; i < l; i++) {
            sum ^= (((unsigned)(name[i]))<<(off&0x1F));
            off += 8;
        }
        hash1 = sum;

        for(sum = off = 0; i < len; i++) {
            temp = (((unsigned)(name[i]))<<(off&0x1F));
            sum ^= temp;
            n = temp & 0x1F;
            sum = (sum << (32-n)) | (sum >> n);  // binary "rotate right"
            off += 8;
        }
        hash = sum;
        hash <<= 32;
        hash += hash1;
        return  hash;
    }
}

using namespace std;
using namespace Bsa;


/// Error handling
void BSAFile::fail(const string &msg)
{
    throw std::runtime_error("BSA Error: " + msg + "\nArchive: " + filename);
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
    assert(!isLoaded);

    namespace bfs = boost::filesystem;
    bfs::ifstream input(bfs::path(filename), std::ios_base::binary);

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
    stringBuf.resize(dirsize-12*filenum);
    input.read(&stringBuf[0], stringBuf.size());

    // Check our position
    assert(input.tellg() == std::streampos(12+dirsize));

    // Calculate the offset of the data buffer. All file offsets are
    // relative to this. 12 header bytes + directory + hash table
    // (skipped)
    size_t fileDataOffset = 12 + dirsize + 8*filenum;

    // Set up the the FileStruct table
    files.resize(filenum);
    for(size_t i=0;i<filenum;i++)
    {
        FileStruct &fs = files[i];
        fs.fileSize = offsets[i*2];
        fs.offset = offsets[i*2+1] + fileDataOffset;
        fs.name = &stringBuf[offsets[2*filenum+i]];

        if(fs.offset + fs.fileSize > fsize)
            fail("Archive contains offsets outside itself");

        // Add the file name to the lookup
        //lookup[fs.name] = i;
    }

    std::uint64_t hash;
    for (size_t i = 0; i < filenum; ++i)
    {
        input.read(reinterpret_cast<char*>(&hash), 8);
        mFiles[hash] = files[i];
    }

    isLoaded = true;
}

/// Get the index of a given file name, or -1 if not found
int BSAFile::getIndex(const char *str) const
{
    std::string name(str);
    boost::algorithm::to_lower(name);
    std::uint64_t hash = getHash(name.c_str());
    std::map<std::uint64_t, FileStruct>::const_iterator iter = mFiles.find(hash);
    if (iter != mFiles.end())
        return 0; // NOTE: this is a bit of a hack, exists() only checks for '-1'
    else
        return -1;
}

/// Open an archive file.
void BSAFile::open(const string &file)
{
    filename = file;
    readHeader();
}

Ogre::DataStreamPtr BSAFile::getFile(const char *file)
{
    assert(file);

    std::string name(file);
    boost::algorithm::to_lower(name);
    std::uint64_t hash = getHash(name.c_str());
    std::map<std::uint64_t, FileStruct>::const_iterator it = mFiles.find(hash);
    if (it != mFiles.end())
    {
        const FileStruct &fs = it->second;
        return openConstrainedFileDataStream (filename.c_str (), fs.offset, fs.fileSize);
    }
        fail("File not found: " + string(file));
}
