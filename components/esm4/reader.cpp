/*
  Copyright (C) 2015-2021 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#include "reader.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#undef DEBUG_GROUPSTACK

#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <iostream> // for debugging
#include <sstream>  // for debugging
#include <iomanip>  // for debugging

#if defined(_MSC_VER)
    #pragma warning (push)
    #pragma warning (disable : 4706)
    #include <boost/iostreams/filter/zlib.hpp>
    #pragma warning (pop)
#else
    #include <boost/iostreams/filter/zlib.hpp>
#endif
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <components/bsa/memorystream.hpp>
#include <components/misc/stringops.hpp>
#include <components/files/constrainedfilestream.hpp>

#include "formid.hpp"

namespace ESM4
{

ReaderContext::ReaderContext() : modIndex(0), recHeaderSize(sizeof(RecordHeader)),
    filePos(0), fileRead(0), recordRead(0), currWorld(0), currCell(0), cellGridValid(false)
{
    currCellGrid.cellId = 0;
    currCellGrid.grid.x = 0;
    currCellGrid.grid.y = 0;
    subRecordHeader.typeId = 0;
    subRecordHeader.dataSize = 0;
}

Reader::Reader(Files::IStreamPtr&& esmStream, const std::string& filename)
    : mEncoder(nullptr), mFileSize(0), mStream(std::move(esmStream))
{
    // used by ESMReader only?
    mCtx.filename = filename;

    mCtx.fileRead = 0;
    mStream->seekg(0, mStream->end);
    mFileSize = mStream->tellg();
    mStream->seekg(20); // go to the start but skip the "TES4" record header

    mSavedStream.reset();

    // determine header size
    std::uint32_t subRecName = 0;
    mStream->read((char*)&subRecName, sizeof(subRecName));
    if (subRecName == 0x52444548) // "HEDR"
        mCtx.recHeaderSize = sizeof(RecordHeader) - 4; // TES4 header size is 4 bytes smaller than TES5 header
    else
        mCtx.recHeaderSize = sizeof(RecordHeader);

    // restart from the beginning (i.e. "TES4" record header)
    mStream->seekg(0, mStream->beg);
#if 0
    unsigned int esmVer = mHeader.mData.version.ui;
    bool isTes4 = esmVer == ESM::VER_080 || esmVer == ESM::VER_100;
    //bool isTes5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_170;
    //bool isFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;

    // TES4 header size is 4 bytes smaller than TES5 header
    mCtx.recHeaderSize = isTes4 ? sizeof(ESM4::RecordHeader) - 4 : sizeof(ESM4::RecordHeader);
#endif
    getRecordHeader();
    if (mCtx.recordHeader.record.typeId == REC_TES4)
    {
        mHeader.load(*this);
        mCtx.fileRead += mCtx.recordHeader.record.dataSize;

        buildLStringIndex(); // for localised strings in Skyrim
    }
    else
        fail("Unknown file format");
}

Reader::~Reader()
{
    close();
}

// Since the record data may have been compressed, it is not always possible to use seek() to
// go to a position of a sub record.
//
// The record header needs to be saved in the context or the header needs to be re-loaded after
// restoring the context. The latter option was chosen.
ReaderContext Reader::getContext()
{
    mCtx.filePos = mStream->tellg();
    mCtx.filePos -= mCtx.recHeaderSize; // update file position
    return mCtx;
}

// NOTE: Assumes that the caller has reopened the file if necessary
bool Reader::restoreContext(const ReaderContext& ctx)
{
    if (mSavedStream) // TODO: doesn't seem to ever happen
    {
        mStream = std::move(mSavedStream);
    }

    mCtx.groupStack.clear(); // probably not necessary since it will be overwritten
    mCtx = ctx;
    mStream->seekg(ctx.filePos); // update file position

    return getRecordHeader();
}

void Reader::close()
{
    mStream.reset();
    //clearCtx();
    //mHeader.blank();
}

void Reader::openRaw(Files::IStreamPtr&& stream, const std::string& filename)
{
    close();

    mStream = std::move(stream);
    mCtx.filename = filename;
    mCtx.fileRead = 0;
    mStream->seekg(0, mStream->end);
    mFileSize = mStream->tellg();
    mStream->seekg(0, mStream->beg);

}

void Reader::open(Files::IStreamPtr&& stream, const std::string &filename)
{
    openRaw(std::move(stream), filename);

    // should at least have the size of ESM3 record header (20 or 24 bytes for ESM4)
    assert (mFileSize >= 16);
    std::uint32_t modVer = 0;
    if (getExact(modVer)) // get the first 4 bytes of the record header only
    {
        // FIXME: need to setup header/context
        if (modVer == REC_TES4)
        {
        }
        else
        {
        }
    }

    throw std::runtime_error("Unknown file format"); // can't yet use fail() as mCtx is not setup
}

void Reader::openRaw(const std::string& filename)
{
    openRaw(Files::openConstrainedFileStream(filename), filename);
}

void Reader::open(const std::string& filename)
{
    open(Files::openConstrainedFileStream(filename), filename);
}

void Reader::setRecHeaderSize(const std::size_t size)
{
    mCtx.recHeaderSize = size;
}

// FIXME: only "English" strings supported for now
void Reader::buildLStringIndex()
{
    if ((mHeader.mFlags & Rec_ESM) == 0 || (mHeader.mFlags & Rec_Localized) == 0)
        return;

    boost::filesystem::path p(mCtx.filename);
    std::string filename = p.stem().filename().string();

    buildLStringIndex("Strings/" + filename + "_English.STRINGS",   Type_Strings);
    buildLStringIndex("Strings/" + filename + "_English.ILSTRINGS", Type_ILStrings);
    buildLStringIndex("Strings/" + filename + "_English.DLSTRINGS", Type_DLStrings);
}

void Reader::buildLStringIndex(const std::string& stringFile, LocalizedStringType stringType)
{
    std::uint32_t numEntries;
    std::uint32_t dataSize;
    std::uint32_t stringId;
    LStringOffset sp;
    sp.type = stringType;

    // TODO: possibly check if the resource exists?
    Files::IStreamPtr filestream = Files::openConstrainedFileStream(stringFile);

    filestream->seekg(0, std::ios::end);
    std::size_t fileSize = filestream->tellg();
    filestream->seekg(0, std::ios::beg);

    std::istream* stream = filestream.get();
    switch (stringType)
    {
        case Type_Strings:   mStrings =   std::move(filestream); break;
        case Type_ILStrings: mILStrings = std::move(filestream); break;
        case Type_DLStrings: mDLStrings = std::move(filestream); break;
        default:
            throw std::runtime_error("ESM4::Reader::unknown localised string type");
    }

    stream->read((char*)&numEntries, sizeof(numEntries));
    stream->read((char*)&dataSize, sizeof(dataSize));
    std::size_t dataStart = fileSize - dataSize;
    for (unsigned int i = 0; i < numEntries; ++i)
    {
        stream->read((char*)&stringId, sizeof(stringId));
        stream->read((char*)&sp.offset, sizeof(sp.offset));
        sp.offset += (std::uint32_t)dataStart;
        mLStringIndex[stringId] = sp;
    }
    //assert (dataStart - stream->tell() == 0 && "String file start of data section mismatch");
}

void Reader::getLocalizedString(std::string& str)
{
    if (!hasLocalizedStrings())
        return (void)getZString(str);

    std::uint32_t stringId; // FormId
    get(stringId);
    if (stringId) // TES5 FoxRace, BOOK
        getLocalizedStringImpl(stringId, str);
}

// FIXME: very messy and probably slow/inefficient
void Reader::getLocalizedStringImpl(const FormId stringId, std::string& str)
{
    const std::map<FormId, LStringOffset>::const_iterator it = mLStringIndex.find(stringId);

    if (it != mLStringIndex.end())
    {
        std::istream* filestream = nullptr;

        switch (it->second.type)
        {
            case Type_Strings: // no string size provided
            {
                filestream = mStrings.get();
                filestream->seekg(it->second.offset);

                char ch;
                std::vector<char> data;
                do {
                    filestream->read(&ch, sizeof(ch));
                    data.push_back(ch);
                } while (ch != 0);

                str = std::string(data.data());
                return;
            }
            case Type_ILStrings: filestream = mILStrings.get(); break;
            case Type_DLStrings: filestream = mDLStrings.get(); break;
            default:
                throw std::runtime_error("ESM4::Reader::getLocalizedString unknown string type");
        }

        // get ILStrings or DLStrings (they provide string size)
        filestream->seekg(it->second.offset);
        std::uint32_t size = 0;
        filestream->read((char*)&size, sizeof(size));
        getStringImpl(str, size, *filestream, mEncoder, true); // expect null terminated string
    }
    else
        throw std::runtime_error("ESM4::Reader::getLocalizedString localized string not found");
}

bool Reader::getRecordHeader()
{
    // FIXME: this seems very hacky but we may have skipped subrecords from within an inflated data block
    if (/*mStream->eof() && */mSavedStream)
    {
        mStream = std::move(mSavedStream);
    }

    mStream->read((char*)&mCtx.recordHeader, mCtx.recHeaderSize);
    std::size_t bytesRead = (std::size_t)mStream->gcount();

    // keep track of data left to read from the file
    mCtx.fileRead += mCtx.recHeaderSize;

    mCtx.recordRead = 0; // for keeping track of sub records

    // After reading the record header we can cache a WRLD or CELL formId for convenient access later.
    // FIXME: currently currWorld and currCell are set manually when loading the WRLD and CELL records

    // HACK: mCtx.groupStack.back() is updated before the record data are read/skipped
    //       N.B. the data must be fully read/skipped for this to work
    if (mCtx.recordHeader.record.typeId != REC_GRUP && !mCtx.groupStack.empty())
    {
        mCtx.groupStack.back().second += (std::uint32_t)mCtx.recHeaderSize + mCtx.recordHeader.record.dataSize;

        // keep track of data left to read from the file
        mCtx.fileRead += mCtx.recordHeader.record.dataSize;
    }

    return bytesRead == mCtx.recHeaderSize;
}

void Reader::getRecordData(bool dump)
{
    std::uint32_t uncompressedSize = 0;

    if ((mCtx.recordHeader.record.flags & Rec_Compressed) != 0)
    {
        mStream->read(reinterpret_cast<char*>(&uncompressedSize), sizeof(std::uint32_t));

        std::size_t recordSize = mCtx.recordHeader.record.dataSize - sizeof(std::uint32_t);
        Bsa::MemoryInputStream compressedRecord(recordSize);
        mStream->read(compressedRecord.getRawData(), recordSize);
        std::istream *fileStream = (std::istream*)&compressedRecord;
        mSavedStream = std::move(mStream);

        mCtx.recordHeader.record.dataSize = uncompressedSize - sizeof(uncompressedSize);

        auto memoryStreamPtr = std::make_unique<Bsa::MemoryInputStream>(uncompressedSize);

        boost::iostreams::filtering_streambuf<boost::iostreams::input> inputStreamBuf;
        inputStreamBuf.push(boost::iostreams::zlib_decompressor());
        inputStreamBuf.push(*fileStream);

        boost::iostreams::basic_array_sink<char> sr(memoryStreamPtr->getRawData(), uncompressedSize);
        boost::iostreams::copy(inputStreamBuf, sr);

    // For debugging only
//#if 0
if (dump)
{
        std::ostringstream ss;
        char* data = memoryStreamPtr->getRawData();
        for (unsigned int i = 0; i < uncompressedSize; ++i)
        {
            if (data[i] > 64 && data[i] < 91)
                ss << (char)(data[i]) << " ";
            else
                ss << std::setfill('0') << std::setw(2) << std::hex << (int)(data[i]);
            if ((i & 0x000f) == 0xf)
                ss << "\n";
            else if (i < uncompressedSize-1)
                ss << " ";
        }
        std::cout << ss.str() << std::endl;
}
//#endif
        mStream = std::make_unique<Files::StreamWithBuffer<Bsa::MemoryInputStream>>(std::move(memoryStreamPtr));
    }
}

void Reader::skipRecordData()
{
    assert (mCtx.recordRead <= mCtx.recordHeader.record.dataSize && "Skipping after reading more than available");
    mStream->ignore(mCtx.recordHeader.record.dataSize - mCtx.recordRead);
    mCtx.recordRead = mCtx.recordHeader.record.dataSize; // for getSubRecordHeader()
}

bool Reader::getSubRecordHeader()
{
    bool result = false;
    // NOTE: some SubRecords have 0 dataSize (e.g. SUB_RDSD in one of REC_REGN records in Oblivion.esm).
    // Also SUB_XXXX has zero dataSize and the following 4 bytes represent the actual dataSize
    // - hence it require manual updtes to mCtx.recordRead via updateRecordRead()
    // See ESM4::NavMesh and ESM4::World.
    if (mCtx.recordHeader.record.dataSize - mCtx.recordRead >= sizeof(mCtx.subRecordHeader))
    {
        result = getExact(mCtx.subRecordHeader);
        // HACK: below assumes sub-record data will be read or skipped in full;
        //       this hack aims to avoid updating mCtx.recordRead each time anything is read
        mCtx.recordRead += (sizeof(mCtx.subRecordHeader) + mCtx.subRecordHeader.dataSize);
    }
    else if (mCtx.recordRead > mCtx.recordHeader.record.dataSize)
    {
        // try to correct any overshoot, seek to the end of the expected data
        // this will only work if mCtx.subRecordHeader.dataSize was fully read or skipped
        // (i.e. it will only correct mCtx.subRecordHeader.dataSize being incorrect)
        // TODO: not tested
        std::uint32_t overshoot = (std::uint32_t)mCtx.recordRead - mCtx.recordHeader.record.dataSize;

        std::size_t pos = mStream->tellg();
        mStream->seekg(pos - overshoot);

        return false;
    }

    return result;
}

void Reader::skipSubRecordData()
{
    mStream->ignore(mCtx.subRecordHeader.dataSize);
}

void Reader::skipSubRecordData(std::uint32_t size)
{
    mStream->ignore(size);
}

void Reader::enterGroup()
{
#ifdef DEBUG_GROUPSTACK
    std::string padding; // FIXME: debugging only
    padding.insert(0, mCtx.groupStack.size()*2, ' ');
    std::cout << padding << "Starting record group "
              << printLabel(mCtx.recordHeader.group.label, mCtx.recordHeader.group.type) << std::endl;
#endif
    // empty group if the group size is same as the header size
    if (mCtx.recordHeader.group.groupSize == (std::uint32_t)mCtx.recHeaderSize)
    {
#ifdef DEBUG_GROUPSTACK
        std::cout << padding << "Ignoring record group " // FIXME: debugging only
            << printLabel(mCtx.recordHeader.group.label, mCtx.recordHeader.group.type)
            << " (empty)" << std::endl;
#endif
        if (!mCtx.groupStack.empty()) // top group may be empty (e.g. HAIR in Skyrim)
        {
            // don't put on the stack, exitGroupCheck() may not get called before recursing into this method
            mCtx.groupStack.back().second += mCtx.recordHeader.group.groupSize;
            exitGroupCheck();
        }

        return; // don't push an empty group, just return
    }

    // push group
    mCtx.groupStack.push_back(std::make_pair(mCtx.recordHeader.group, (std::uint32_t)mCtx.recHeaderSize));
}

void Reader::exitGroupCheck()
{
    if (mCtx.groupStack.empty())
        return;

    // pop finished groups (note reading too much is allowed here)
    std::uint32_t lastGroupSize = mCtx.groupStack.back().first.groupSize;
    while (mCtx.groupStack.back().second >= lastGroupSize)
    {
#ifdef DEBUG_GROUPSTACK
        GroupTypeHeader grp = mCtx.groupStack.back().first; // FIXME: grp is for debugging only
#endif
        // try to correct any overshoot
        // TODO: not tested
        std::uint32_t overshoot = mCtx.groupStack.back().second - lastGroupSize;
        if (overshoot > 0)
        {
            std::size_t pos = mStream->tellg();
            mStream->seekg(pos - overshoot);
        }

        mCtx.groupStack.pop_back();
#ifdef DEBUG_GROUPSTACK
        std::string padding; // FIXME: debugging only
        padding.insert(0, mCtx.groupStack.size()*2, ' ');
        std::cout << padding << "Finished record group " << printLabel(grp.label, grp.type) << std::endl;
#endif
        // if the previous group was the final one no need to do below
        if (mCtx.groupStack.empty())
            return;

        mCtx.groupStack.back().second += lastGroupSize;
        lastGroupSize = mCtx.groupStack.back().first.groupSize;

        assert (lastGroupSize >= mCtx.groupStack.back().second && "Read more records than available");
//#if 0
        if (mCtx.groupStack.back().second > lastGroupSize) // FIXME: debugging only
            std::cerr << printLabel(mCtx.groupStack.back().first.label,
                                          mCtx.groupStack.back().first.type)
                      << " read more records than available" << std::endl;
//#endif
    }
}

// WARNING: this method should be used after first calling enterGroup()
// else the method may try to dereference an element that does not exist
const GroupTypeHeader& Reader::grp(std::size_t pos) const
{
    assert (pos <= mCtx.groupStack.size()-1 && "ESM4::Reader::grp - exceeded stack depth");

    return (*(mCtx.groupStack.end()-pos-1)).first;
}

void Reader::skipGroupData()
{
    assert (!mCtx.groupStack.empty() && "Skipping group with an empty stack");

    // subtract what was already read/skipped
    std::uint32_t skipSize = mCtx.groupStack.back().first.groupSize - mCtx.groupStack.back().second;

    mStream->ignore(skipSize);

    // keep track of data left to read from the file
    mCtx.fileRead += skipSize;

    mCtx.groupStack.back().second = mCtx.groupStack.back().first.groupSize;
}

void Reader::skipGroup()
{
#ifdef DEBUG_GROUPSTACK
    std::string padding; // FIXME: debugging only
    padding.insert(0, mCtx.groupStack.size()*2, ' ');
    std::cout << padding << "Skipping record group "
              << printLabel(mCtx.recordHeader.group.label, mCtx.recordHeader.group.type) << std::endl;
#endif
    // subtract the size of header already read before skipping
    std::uint32_t skipSize = mCtx.recordHeader.group.groupSize - (std::uint32_t)mCtx.recHeaderSize;
    mStream->ignore(skipSize);

    // keep track of data left to read from the file
    mCtx.fileRead += skipSize;

    // NOTE: mCtx.groupStack.back().second already has mCtx.recHeaderSize from enterGroup()
    if (!mCtx.groupStack.empty())
        mCtx.groupStack.back().second += mCtx.recordHeader.group.groupSize;
}

const CellGrid& Reader::currCellGrid() const
{
    // Maybe should throw an exception instead?
    assert (mCtx.cellGridValid && "Attempt to use an invalid cell grid");

    return mCtx.currCellGrid;
}

// NOTE: the parameter 'files' must have the file names in the loaded order
void Reader::updateModIndices(const std::vector<std::string>& files)
{
    if (files.size() >= 0xff)
        throw std::runtime_error("ESM4::Reader::updateModIndices too many files"); // 0xff is reserved

    // NOTE: this map is rebuilt each time this method is called (i.e. each time a file is loaded)
    // Perhaps there is an opportunity to optimize this by saving the result somewhere.
    // But then, the number of files is at most around 250 so perhaps keeping it simple might be better.

    // build a lookup map
    std::unordered_map<std::string, size_t> fileIndex;

    for (size_t i = 0; i < files.size(); ++i) // ATTENTION: assumes current file is not included
        fileIndex[Misc::StringUtils::lowerCase(files[i])] = i;

    mCtx.parentFileIndices.resize(mHeader.mMaster.size());
    for (unsigned int i = 0; i < mHeader.mMaster.size(); ++i)
    {
        // locate the position of the dependency in already loaded files
        std::unordered_map<std::string, size_t>::const_iterator it
            = fileIndex.find(Misc::StringUtils::lowerCase(mHeader.mMaster[i].name));

        if (it != fileIndex.end())
            mCtx.parentFileIndices[i] = (std::uint32_t)((it->second << 24) & 0xff000000);
        else
            throw std::runtime_error("ESM4::Reader::updateModIndices required dependency file not loaded");
#if 0
        std::cout << "Master Mod: " << mCtx.header.mMaster[i].name << ", " // FIXME: debugging only
                  << formIdToString(mCtx.parentFileIndices[i]) << std::endl;
#endif
    }

    if (!mCtx.parentFileIndices.empty() &&  mCtx.parentFileIndices[0] != 0)
        throw std::runtime_error("ESM4::Reader::updateModIndices base modIndex is not zero");
}

// ModIndex adjusted formId according to master file dependencies
// (see http://www.uesp.net/wiki/Tes4Mod:FormID_Fixup)
// NOTE: need to update modindex to parentFileIndices.size() before saving
//
// FIXME: probably should add a parameter to check for mCtx.header::mOverrides
//        (ACHR, LAND, NAVM, PGRE, PHZD, REFR), but not sure what exactly overrides mean
//        i.e. use the modindx of its master?
// FIXME: Apparently ModIndex '00' in an ESP means the object is defined in one of its masters.
//        This means we may need to search multiple times to get the correct id.
//        (see https://www.uesp.net/wiki/Tes4Mod:Formid#ModIndex_Zero)
void Reader::adjustFormId(FormId& id)
{
    if (mCtx.parentFileIndices.empty())
        return;

    std::size_t index = (id >> 24) & 0xff;

    if (index < mCtx.parentFileIndices.size())
        id = mCtx.parentFileIndices[index] | (id & 0x00ffffff);
    else
        id = mCtx.modIndex | (id & 0x00ffffff);
}

bool Reader::getFormId(FormId& id)
{
    if (!getExact(id))
        return false;

    adjustFormId(id);
    return true;
}

void Reader::adjustGRUPFormId()
{
    adjustFormId(mCtx.recordHeader.group.label.value);
}

[[noreturn]] void Reader::fail(const std::string& msg)
{
    std::stringstream ss;

    ss << "ESM Error: " << msg;
    ss << "\n  File: " << mCtx.filename;
    ss << "\n  Record: " << ESM::printName(mCtx.recordHeader.record.typeId);
    ss << "\n  Subrecord: " << ESM::printName(mCtx.subRecordHeader.typeId);
    if (mStream.get())
        ss << "\n  Offset: 0x" << std::hex << mStream->tellg();

    throw std::runtime_error(ss.str());
}

}
