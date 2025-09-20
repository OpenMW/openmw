/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (bsafile.cpp) is part of the OpenMW package.

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

#include "bsafile.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <system_error>

#include <components/esm/fourcc.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/utils.hpp>

using namespace Bsa;

/// Error handling
[[noreturn]] void BSAFile::fail(const std::string& msg) const
{
    throw std::runtime_error("BSA Error: " + msg + "\nArchive: " + Files::pathToUnicodeString(mFilepath));
}

// the getHash code is from bsapack from ghostwheel
// the code is also the same as in
// https://github.com/arviceblot/bsatool_rs/commit/67cb59ec3aaeedc0849222ea387f031c33e48c81
BSAFile::Hash getHash(const std::string& name)
{
    BSAFile::Hash hash;
    unsigned l = (static_cast<unsigned>(name.size()) >> 1);
    unsigned sum, off, temp, i, n;

    for (sum = off = i = 0; i < l; i++)
    {
        sum ^= (((unsigned)(name[i])) << (off & 0x1F));
        off += 8;
    }
    hash.mLow = sum;

    for (sum = off = 0; i < name.size(); i++)
    {
        temp = (((unsigned)(name[i])) << (off & 0x1F));
        sum ^= temp;
        n = temp & 0x1F;
        sum = (sum << (32 - n)) | (sum >> n); // binary "rotate right"
        off += 8;
    }
    hash.mHigh = sum;
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

    std::ifstream input(mFilepath, std::ios_base::binary);

    // Total archive size
    const std::streamsize fsize = Files::getStreamSizeLeft(input);

    if (fsize < 12)
        fail("File too small to be a valid BSA archive");

    // Get essential header numbers
    size_t dirsize, filenum;
    {
        // First 12 bytes
        uint32_t head[3];

        input.read(reinterpret_cast<char*>(head), 12);

        if (input.fail())
            fail(std::format("Failed to read head: {}", std::generic_category().message(errno)));

        if (head[0] != 0x100)
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
    if ((filenum * 21 > unsigned(fsize - 12)) || (dirsize + 8 * filenum > unsigned(fsize - 12)))
        fail("Directory information larger than entire archive");

    // Read the offset info into a temporary buffer
    std::vector<uint32_t> offsets(3 * filenum);
    input.read(reinterpret_cast<char*>(offsets.data()), 12 * filenum);

    if (input.fail())
        fail(std::format("Failed to read offsets: {}", std::generic_category().message(errno)));

    // Read the string table
    mStringBuf.resize(dirsize - 12 * filenum);
    input.read(mStringBuf.data(), mStringBuf.size());

    if (input.fail())
        fail(std::format("Failed to read string table: {}", std::generic_category().message(errno)));

    // Check our position
    assert(input.tellg() == std::streampos(12 + dirsize));
    std::vector<Hash> hashes(filenum);
    static_assert(sizeof(Hash) == 8);
    input.read(reinterpret_cast<char*>(hashes.data()), 8 * filenum);

    if (input.fail())
        fail(std::format("Failed to read hashes: {}", std::generic_category().message(errno)));

    // Calculate the offset of the data buffer. All file offsets are
    // relative to this. 12 header bytes + directory + hash table
    // (skipped)
    size_t fileDataOffset = 12 + dirsize + 8 * filenum;

    // Set up the the FileStruct table
    mFiles.reserve(filenum);
    size_t endOfNameBuffer = 0;
    for (size_t i = 0; i < filenum; i++)
    {
        FileStruct& fs = mFiles.emplace_back();

        const uint32_t fileSize = offsets[i * 2];
        const uint32_t offset = offsets[i * 2 + 1] + static_cast<uint32_t>(fileDataOffset);

        if (fileSize + offset > fsize)
            fail("Archive contains offsets outside itself");

        const uint32_t nameOffset = offsets[2 * filenum + i];

        if (nameOffset >= mStringBuf.size())
            fail("Archive contains names offset outside itself");

        const char* const begin = mStringBuf.data() + nameOffset;
        const char* const end = reinterpret_cast<const char*>(std::memchr(begin, '\0', mStringBuf.size() - nameOffset));

        if (end == nullptr)
            fail("Archive contains non-zero terminated string");

        const std::size_t nameSize = end - begin;

        fs.mFileSize = fileSize;
        fs.mOffset = offset;
        fs.mHash = hashes[i];
        fs.mNameOffset = nameOffset;
        fs.mNameSize = static_cast<uint32_t>(nameSize);
        fs.mNamesBuffer = &mStringBuf;

        endOfNameBuffer = std::max(endOfNameBuffer, nameOffset + nameSize + 1);
        assert(endOfNameBuffer <= mStringBuf.size());
    }
    mStringBuf.resize(endOfNameBuffer);

    std::sort(mFiles.begin(), mFiles.end(),
        [](const FileStruct& left, const FileStruct& right) { return left.mOffset < right.mOffset; });

    mIsLoaded = true;
}

/// Write header information to the output sink
void Bsa::BSAFile::writeHeader()
{
    std::fstream output(mFilepath, std::ios::binary | std::ios::in | std::ios::out);

    uint32_t head[3];
    head[0] = 0x100;
    auto fileDataOffset = mFiles.empty() ? 12 : mFiles.front().mOffset;
    head[1] = static_cast<uint32_t>(fileDataOffset - 12 - 8 * mFiles.size());

    output.seekp(0, std::ios_base::end);

    head[2] = static_cast<uint32_t>(mFiles.size());
    output.seekp(0);
    output.write(reinterpret_cast<char*>(head), 12);

    std::sort(mFiles.begin(), mFiles.end(), [](const FileStruct& left, const FileStruct& right) {
        return std::make_pair(left.mHash.mLow, left.mHash.mHigh) < std::make_pair(right.mHash.mLow, right.mHash.mHigh);
    });

    size_t filenum = mFiles.size();
    std::vector<uint32_t> offsets(3 * filenum);
    std::vector<Hash> hashes(filenum);
    for (size_t i = 0; i < filenum; i++)
    {
        auto& f = mFiles[i];
        offsets[i * 2] = f.mFileSize;
        offsets[i * 2 + 1] = f.mOffset - fileDataOffset;
        offsets[2 * filenum + i] = f.mNameOffset;
        hashes[i] = f.mHash;
    }
    output.write(reinterpret_cast<char*>(offsets.data()), sizeof(uint32_t) * offsets.size());
    output.write(reinterpret_cast<char*>(mStringBuf.data()), mStringBuf.size());
    output.seekp(fileDataOffset - 8 * mFiles.size(), std::ios_base::beg);
    output.write(reinterpret_cast<char*>(hashes.data()), sizeof(Hash) * hashes.size());
}

/// Open an archive file.
void BSAFile::open(const std::filesystem::path& file)
{
    if (mIsLoaded)
        close();

    mFilepath = file;
    if (std::filesystem::exists(file))
        readHeader();
    else
    {
        {
            std::fstream(mFilepath, std::ios::binary | std::ios::out);
        }
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
    mIsLoaded = false;
}

Files::IStreamPtr Bsa::BSAFile::getFile(const FileStruct* file)
{
    return Files::openConstrainedFileStream(mFilepath, file->mOffset, file->mFileSize);
}

void Bsa::BSAFile::addFile(const std::string& filename, std::istream& file)
{
    if (!mIsLoaded)
        fail("Unable to add file " + filename + " the archive is not opened");

    auto newStartOfDataBuffer = 12 + (12 + 8) * (mFiles.size() + 1) + mStringBuf.size() + filename.size() + 1;
    if (mFiles.empty())
        std::filesystem::resize_file(mFilepath, newStartOfDataBuffer);

    std::fstream stream(mFilepath, std::ios::binary | std::ios::in | std::ios::out);

    FileStruct newFile;
    file.seekg(0, std::ios::end);
    newFile.mFileSize = static_cast<uint32_t>(file.tellg());
    newFile.mHash = getHash(filename);

    if (mFiles.empty())
        newFile.mOffset = static_cast<uint32_t>(newStartOfDataBuffer);
    else
    {
        std::vector<char> buffer;
        while (mFiles.front().mOffset < newStartOfDataBuffer)
        {
            FileStruct& firstFile = mFiles.front();
            buffer.resize(firstFile.mFileSize);

            stream.seekg(firstFile.mOffset, std::ios::beg);
            stream.read(buffer.data(), firstFile.mFileSize);

            stream.seekp(0, std::ios::end);
            firstFile.mOffset = static_cast<uint32_t>(stream.tellp());

            stream.write(buffer.data(), firstFile.mFileSize);

            // ensure sort order is preserved
            std::rotate(mFiles.begin(), mFiles.begin() + 1, mFiles.end());
        }
        stream.seekp(0, std::ios::end);
        newFile.mOffset = static_cast<uint32_t>(stream.tellp());
    }

    newFile.mNameOffset = mStringBuf.size();
    newFile.mNameSize = filename.size();
    newFile.mNamesBuffer = &mStringBuf;

    mStringBuf.insert(mStringBuf.end(), filename.begin(), filename.end());
    mStringBuf.push_back('\0');

    mFiles.push_back(newFile);

    mHasChanged = true;

    stream.seekp(0, std::ios::end);
    file.seekg(0, std::ios::beg);
    stream << file.rdbuf();
}

BsaVersion Bsa::BSAFile::detectVersion(const std::filesystem::path& filePath)
{
    std::ifstream input(filePath, std::ios_base::binary);

    // Get essential header numbers

    // First 12 bytes
    uint32_t head[3];

    input.read(reinterpret_cast<char*>(head), sizeof(head));

    if (input.gcount() != sizeof(head))
        return BsaVersion::Unknown;

    if (head[0] == static_cast<uint32_t>(BsaVersion::Uncompressed))
    {
        return BsaVersion::Uncompressed;
    }

    if (head[0] == static_cast<uint32_t>(BsaVersion::Compressed))
    {
        return BsaVersion::Compressed;
    }

    if (head[0] == ESM::fourCC("BTDX"))
    {
        if (head[2] == ESM::fourCC("GNRL"))
            return BsaVersion::BA2GNRL;
        if (head[2] == ESM::fourCC("DX10"))
            return BsaVersion::BA2DX10;
    }

    return BsaVersion::Unknown;
}
