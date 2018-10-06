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

  TES4 stuff added by cc9cii 2018

 */
#include "tes4bsa_file.hpp"

#include <stdexcept>
#include <cassert>

#include <boost/scoped_array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <zlib.h>

#include <extern/BSAOpt/hash.hpp> // see: http://en.uesp.net/wiki/Tes4Mod:Hash_Calculation

#undef TEST_UNIQUE_HASH

namespace
{
    void getBZString(std::string& str, boost::filesystem::ifstream& filestream)
    {
        char size = 0;
        filestream.read(&size, 1);

        boost::scoped_array<char> buf(new char[size]);
        filestream.read(buf.get(), size);

        if (buf[size - 1] != 0)
            str.assign(buf.get(), size);
        else
            str.assign(buf.get(), size - 1); // don't copy null terminator
        assert((size_t)size-1 == str.size() && "getBZString string size mismatch");
        return;
    }
}

using namespace Bsa;


/// Error handling
void TES4BSAFile::fail(const std::string& msg)
{
    throw std::runtime_error("BSA Error: " + msg + "\nArchive: " + mFilename);
}

/// Read header information from the input source
void TES4BSAFile::readHeader()
{
    assert(!isLoaded);

    namespace bfs = boost::filesystem;
    bfs::ifstream input(bfs::path(mFilename), std::ios_base::binary);

    // Total archive size
    std::streamoff fsize = 0;
    if(input.seekg(0, std::ios_base::end))
    {
        fsize = input.tellg();
        input.seekg(0);
    }

    if(fsize < 36) // header is 36 bytes
        fail("File too small to be a valid BSA archive");

    // Get essential header numbers
    //size_t dirsize, filenum;
    std::uint32_t archiveFlags, folderCount, fileCount, totalFileNameLength;
    {
        // First 36 bytes
        std::uint32_t header[9];

        input.read(reinterpret_cast<char*>(header), 36);

        if(header[0] != 0x00415342 /*"BSA\x00"*/ || (header[1] != 0x67 /*TES4*/ && header[1] != 0x68 /*TES5*/))
            fail("Unrecognized TES4 BSA header");

        // header[2] is offset, should be 36 = 0x24 which is the size of the header

        // Oblivion - Meshes.bsa
        //
        // 0111 1000 0111 = 0x0787
        //  ^^^ ^     ^^^
        //  ||| |     ||+-- has names for dirs  (mandatory?)
        //  ||| |     |+--- has names for files (mandatory?)
        //  ||| |     +---- files are compressed by default
        //  ||| |
        //  ||| +---------- unknown (TES5: retain strings during startup)
        //  ||+------------ unknown (TES5: embedded file names)
        //  |+------------- unknown
        //  +-------------- unknown
        //
        archiveFlags          = header[3];
        folderCount           = header[4];
        fileCount             = header[5];
      //totalFolderNameLength = header[6];
        totalFileNameLength   = header[7];
      //fileFlags             = header[8]; // FIXME: an opportunity to optimize here

        mCompressedByDefault = (archiveFlags & 0x4) != 0;
        mEmbeddedFileNames = header[1] == 0x68 /*TES5*/ && (archiveFlags & 0x100) != 0;
    }

    // TODO: more checks for BSA file corruption

    // folder records
    std::uint64_t hash;
    FolderRecord fr;
    for (std::uint32_t i = 0; i < folderCount; ++i)
    {
        input.read(reinterpret_cast<char*>(&hash), 8);
        input.read(reinterpret_cast<char*>(&fr.count), 4); // not sure purpose of count
        input.read(reinterpret_cast<char*>(&fr.offset), 4); // not sure purpose of offset

        std::map<std::uint64_t, FolderRecord>::const_iterator lb = mFolders.lower_bound(hash);
        if (lb != mFolders.end() && !(mFolders.key_comp()(hash, lb->first)))
            fail("Archive found duplicate folder name hash");
        else
            mFolders.insert(lb, std::pair<std::uint64_t, FolderRecord>(hash, fr));
    }

    // file record blocks
    std::uint64_t fileHash;
    FileRecord file;

    std::string folder("");
    std::uint64_t folderHash;
    if ((archiveFlags & 0x1) == 0)
        folderCount = 1; // TODO: not tested

    for (std::uint32_t i = 0; i < folderCount; ++i)
    {
        if ((archiveFlags & 0x1) != 0)
            getBZString(folder, input);

        folderHash = GenOBHash(folder, std::string(""));

        std::map<std::uint64_t, FolderRecord>::iterator iter = mFolders.find(folderHash);
        if (iter == mFolders.end())
            fail("Archive folder name hash not found");

        for (std::uint32_t j = 0; j < iter->second.count; ++j)
        {
            input.read(reinterpret_cast<char*>(&fileHash), 8);
            input.read(reinterpret_cast<char*>(&file.size), 4);
            input.read(reinterpret_cast<char*>(&file.offset), 4);

            std::map<std::uint64_t, FileRecord>::const_iterator lb = iter->second.files.lower_bound(fileHash);
            if (lb != iter->second.files.end() && !(iter->second.files.key_comp()(fileHash, lb->first)))
                fail("Archive found duplicate file name hash");

            iter->second.files.insert(lb, std::pair<std::uint64_t, FileRecord>(fileHash, file));
        }
    }

    // file record blocks
    if ((archiveFlags & 0x2) != 0)
    {
        mStringBuf.resize(totalFileNameLength);
        input.read(&mStringBuf[0], mStringBuf.size()); // TODO: maybe useful in building a lookup map?
    }

    // TODO: more checks for BSA file corruption

    isLoaded = true;
}

TES4BSAFile::FileRecord TES4BSAFile::getFileRecord(const std::string& str) const
{
    boost::filesystem::path p(str);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    std::string filename = p.filename().string();
    p.remove_filename();

    std::string folder = p.string();
    // GenOBHash already converts to lowercase and replaces file separators but not for path
    boost::algorithm::to_lower(folder);
    std::replace(folder.begin(), folder.end(), '/', '\\');

    std::uint64_t folderHash = GenOBHash(folder, std::string(""));

    std::map<std::uint64_t, FolderRecord>::const_iterator it = mFolders.find(folderHash);
    if (it == mFolders.end())
        return FileRecord(); // folder not found, return default which has offset of -1

    boost::algorithm::to_lower(stem);
    boost::algorithm::to_lower(ext);
    std::uint64_t fileHash = GenOBHashPair(stem, ext);
    std::map<std::uint64_t, FileRecord>::const_iterator iter = it->second.files.find(fileHash);
    if (iter == it->second.files.end())
        return FileRecord(); // file not found, return default which has offset of -1

    // cache for next time
    std::uint64_t hash = GenOBHash(folder, filename);

#if defined (TEST_UNIQUE_HASH)
    FileList::const_iterator lb = mFiles.lower_bound(hash);
    if (lb != mFiles.end() && !(mFiles.key_comp()(hash, lb->first)))
    {
        // found, check if same filename
        if (lb->second.fileName == str)
            return iter->second; // same name, should not have got here!!
        else
        {
            // different filename, hash is not unique!
            std::cerr << "BSA hash collision: " << str << std::hex << "0x" << hash << std::endl;

            return iter->second; // return without cashing
        }
    }

    // not found, cache for later
    const_cast<FileList&>(mFiles).insert(lb, std::pair<std::uint64_t, FileRecord>(hash, iter->second));
    const_cast<FileList&>(mFiles)[hash].fileName = str;
#else
    const_cast<FileList&>(mFiles)[hash] = iter->second; // NOTE: const hack
#endif
    return iter->second;
}

bool TES4BSAFile::exists(const std::string& str) const
{
    // check cache first
    boost::filesystem::path p(str);
    std::string filename = p.filename().string();
    p.remove_filename();

    std::string folder = p.string();
    // GenOBHash already converts to lowercase and replaces file separators but not for path
    boost::algorithm::to_lower(folder);
    std::replace(folder.begin(), folder.end(), '/', '\\');

    std::uint64_t hash = GenOBHash(folder, filename);

    std::map<std::uint64_t, FileRecord>::const_iterator it = mFiles.find(hash);
#if defined (TEST_UNIQUE_HASH)
    if (it != mFiles.end() && it->second.fileName == str)
#else
    if (it != mFiles.end())
#endif
        return true;
    else
        return getFileRecord(str).offset != -1;
}

void TES4BSAFile::open(const std::string& file)
{
    mFilename = file;
    readHeader();
}

Ogre::DataStreamPtr TES4BSAFile::getFile(const std::string& file)
{
    assert(file);

    FileRecord fileRec = getFileRecord(file);
    if(fileRec.offset == -1)
        fail("File not found: " + std::string(file));

    boost::filesystem::ifstream input(boost::filesystem::path(mFilename), std::ios_base::binary);
    input.seekg(fileRec.offset);

    std::string fullPath;
    if (mEmbeddedFileNames)
        getBZString(fullPath, input); // TODO: maybe cache the hash and/or offset of frequently used ones?

    if (( mCompressedByDefault && (fileRec.size & (1<<30)) == 0)
        ||
        (!mCompressedByDefault && (fileRec.size & (1<<30)) != 0))
    {
        std::uint32_t bufSize = 0;
        boost::scoped_array<unsigned char> inBuf;
        inBuf.reset(new unsigned char[fileRec.size-4]);
        input.read(reinterpret_cast<char*>(&bufSize), 4);
        input.read(reinterpret_cast<char*>(inBuf.get()), fileRec.size-4);
        Ogre::MemoryDataStream *outBuf = new Ogre::MemoryDataStream(bufSize);
        Ogre::SharedPtr<Ogre::DataStream> streamPtr(outBuf);

        int ret;
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = bufSize;
        strm.next_in = inBuf.get();
        ret = inflateInit(&strm);
        if (ret != Z_OK)
            throw std::runtime_error("TES4BSAFile::getFile - inflateInit failed");

        strm.avail_out = bufSize;
        strm.next_out = outBuf->getPtr();
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR && "TES4BSAFile::getFile - inflate - state clobbered");
        switch (ret)
        {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR; /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&strm);
            throw std::runtime_error("TES4BSAFile::getFile - inflate failed");
        }
        assert(ret == Z_OK || ret == Z_STREAM_END);
        inflateEnd(&strm);

        return streamPtr;
    }
    else // not compressed TODO: not tested
    {
        Ogre::MemoryDataStream *outBuf = new Ogre::MemoryDataStream(fileRec.size);
        Ogre::SharedPtr<Ogre::DataStream> streamPtr(outBuf);
        input.read(reinterpret_cast<char*>(outBuf->getPtr()), fileRec.size);

        return streamPtr;
    }
}
