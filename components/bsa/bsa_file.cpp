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
#include <boost/filesystem/operations.hpp>

using namespace Bsa;


/// Error handling
[[noreturn]] void BSAFile::fail(const std::string &msg)
{
    throw std::runtime_error("BSA Error: " + msg + "\nArchive: " + mFilename);
}

//the getHash code is from bsapack from ghostwheel
//the code is also the same as in https://github.com/arviceblot/bsatool_rs/commit/67cb59ec3aaeedc0849222ea387f031c33e48c81
BSAFile::Hash getHash(const std::string& name)
{
    BSAFile::Hash hash;
    unsigned l = (static_cast<unsigned>(name.size()) >> 1);
    unsigned sum, off, temp, i, n;

    for (sum = off = i = 0; i < l; i++) {
        sum ^= (((unsigned)(name[i])) << (off & 0x1F));
        off += 8;
    }
    hash.low = sum;

    for (sum = off = 0; i < name.size(); i++) {
        temp = (((unsigned)(name[i])) << (off & 0x1F));
        sum ^= temp;
        n = temp & 0x1F;
        sum = (sum << (32 - n)) | (sum >> n);  // binary "rotate right"
        off += 8;
    }
    hash.high = sum;
    return hash;
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
    input.read(reinterpret_cast<char*>(offsets.data()), 12*filenum);

    // Read the string table
    mStringBuf.resize(dirsize-12*filenum);
    input.read(mStringBuf.data(), mStringBuf.size());

    // Check our position
    assert(input.tellg() == std::streampos(12+dirsize));
    std::vector<Hash> hashes(filenum);
    static_assert(sizeof(Hash) == 8);
    input.read(reinterpret_cast<char*>(hashes.data()), 8*filenum);

    // Calculate the offset of the data buffer. All file offsets are
    // relative to this. 12 header bytes + directory + hash table
    // (skipped)
    size_t fileDataOffset = 12 + dirsize + 8*filenum;

    // Set up the the FileStruct table
    mFiles.resize(filenum);
    size_t endOfNameBuffer = 0;
    for(size_t i=0;i<filenum;i++)
    {
        FileStruct &fs = mFiles[i];
        fs.fileSize = offsets[i*2];
        fs.offset = static_cast<uint32_t>(offsets[i*2+1] + fileDataOffset);
        auto namesOffset = offsets[2*filenum+i];
        fs.setNameInfos(namesOffset, &mStringBuf);
        fs.hash = hashes[i];

        if (namesOffset >= mStringBuf.size()) {
            fail("Archive contains names offset outside itself");
        }
        const void* end = std::memchr(fs.name(), '\0', mStringBuf.size()-namesOffset);
        if (!end) {
            fail("Archive contains non-zero terminated string");
        }

        endOfNameBuffer = std::max(endOfNameBuffer, namesOffset + std::strlen(fs.name())+1);
        assert(endOfNameBuffer <= mStringBuf.size());

        if(fs.offset + fs.fileSize > fsize)
            fail("Archive contains offsets outside itself");

    }
    mStringBuf.resize(endOfNameBuffer);

    std::sort(mFiles.begin(), mFiles.end(), [](const FileStruct& left, const FileStruct& right) {
        return left.offset < right.offset;
    });

    for (size_t i = 0; i < filenum; i++)
    {
        FileStruct& fs = mFiles[i];
        // Add the file name to the lookup
        mLookup[fs.name()] = i;
    }

    mIsLoaded = true;
}

/// Write header information to the output sink
void Bsa::BSAFile::writeHeader()
{
    namespace bfs = boost::filesystem;
    bfs::fstream output(mFilename, std::ios::binary | std::ios::in | std::ios::out);

    uint32_t head[3];
    head[0] = 0x100;
    auto fileDataOffset = mFiles.empty() ? 12 : mFiles.front().offset;
    head[1] = static_cast<uint32_t>(fileDataOffset - 12 - 8*mFiles.size());

    output.seekp(0, std::ios_base::end);

    head[2] = static_cast<uint32_t>(mFiles.size());
    output.seekp(0);
    output.write(reinterpret_cast<char*>(head), 12);

    std::sort(mFiles.begin(), mFiles.end(), [](const FileStruct& left, const FileStruct& right) {
        return std::make_pair(left.hash.low, left.hash.high) < std::make_pair(right.hash.low, right.hash.high);
    });

    size_t filenum = mFiles.size();
    std::vector<uint32_t> offsets(3* filenum);
    std::vector<Hash> hashes(filenum);
    for(size_t i=0;i<filenum;i++)
    {
        auto& f = mFiles[i];
        offsets[i*2] = f.fileSize;
        offsets[i*2+1] = f.offset - fileDataOffset;
        offsets[2*filenum+i] = f.namesOffset;
        hashes[i] = f.hash;
    }
    output.write(reinterpret_cast<char*>(offsets.data()), sizeof(uint32_t)*offsets.size());
    output.write(reinterpret_cast<char*>(mStringBuf.data()), mStringBuf.size());
    output.seekp(fileDataOffset - 8*mFiles.size(), std::ios_base::beg);
    output.write(reinterpret_cast<char*>(hashes.data()), sizeof(Hash)*hashes.size());
}

/// Get the index of a given file name, or -1 if not found
int BSAFile::getIndex(const char *str) const
{
    auto it = mLookup.find(str);
    if(it == mLookup.end())
        return -1;

    size_t res = it->second;
    assert(res < mFiles.size());
    return static_cast<int>(res);
}

/// Open an archive file.
void BSAFile::open(const std::string &file)
{
    if (mIsLoaded)
        close();

    mFilename = file;
    if(boost::filesystem::exists(file))
        readHeader();
    else
    {
        { boost::filesystem::fstream(mFilename, std::ios::binary | std::ios::out); }
        writeHeader();
        mIsLoaded = true;
    }
}

/// Close the archive, write the updated headers to the file
void Bsa::BSAFile::close()
{
    if (mHasChanged)
        writeHeader();

    mFiles.clear();
    mStringBuf.clear();
    mLookup.clear();
    mIsLoaded = false;
}

Files::IStreamPtr BSAFile::getFile(const char *file)
{
    assert(file);
    int i = getIndex(file);
    if(i == -1)
        fail("File not found: " + std::string(file));

    const FileStruct &fs = mFiles[i];

    return Files::openConstrainedFileStream (mFilename.c_str (), fs.offset, fs.fileSize);
}

Files::IStreamPtr BSAFile::getFile(const FileStruct *file)
{
    return Files::openConstrainedFileStream (mFilename.c_str (), file->offset, file->fileSize);
}

void Bsa::BSAFile::addFile(const std::string& filename, std::istream& file)
{
    if (!mIsLoaded)
        fail("Unable to add file " + filename + " the archive is not opened");
    namespace bfs = boost::filesystem;

    auto newStartOfDataBuffer = 12 + (12 + 8) * (mFiles.size() + 1) + mStringBuf.size() + filename.size() + 1;
    if (mFiles.empty())
        bfs::resize_file(mFilename, newStartOfDataBuffer);

    bfs::fstream stream(mFilename, std::ios::binary | std::ios::in | std::ios::out);

    FileStruct newFile;
    file.seekg(0, std::ios::end);
    newFile.fileSize = static_cast<uint32_t>(file.tellg());
    newFile.setNameInfos(mStringBuf.size(), &mStringBuf);
    newFile.hash = getHash(filename);

    if(mFiles.empty())
        newFile.offset = static_cast<uint32_t>(newStartOfDataBuffer);
    else
    {
        std::vector<char> buffer;
        while (mFiles.front().offset < newStartOfDataBuffer) {
            FileStruct& firstFile = mFiles.front();
            buffer.resize(firstFile.fileSize);

            stream.seekg(firstFile.offset, std::ios::beg);
            stream.read(buffer.data(), firstFile.fileSize);

            stream.seekp(0, std::ios::end);
            firstFile.offset = static_cast<uint32_t>(stream.tellp());

            stream.write(buffer.data(), firstFile.fileSize);

            //ensure sort order is preserved
            std::rotate(mFiles.begin(), mFiles.begin() + 1, mFiles.end());
        }
        stream.seekp(0, std::ios::end);
        newFile.offset = static_cast<uint32_t>(stream.tellp());
    }

    mStringBuf.insert(mStringBuf.end(), filename.begin(), filename.end());
    mStringBuf.push_back('\0');
    mFiles.push_back(newFile);

    mHasChanged = true;

    mLookup[filename.c_str()] = mFiles.size() - 1;

    stream.seekp(0, std::ios::end);
    file.seekg(0, std::ios::beg);
    stream << file.rdbuf();
}
