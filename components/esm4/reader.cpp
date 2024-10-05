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

#undef DEBUG_GROUPSTACK

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>

#include <zlib.h>

#include <components/bsa/memorystream.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/to_utf8/to_utf8.hpp>
#include <components/vfs/manager.hpp>

#include "grouptype.hpp"

namespace ESM4
{
    namespace
    {
        using FormId = ESM::FormId;
        using FormId32 = ESM::FormId32;

        std::string getError(const std::string& header, const int errorCode, const char* msg)
        {
            return header + ": code " + std::to_string(errorCode) + ", " + std::string(msg != nullptr ? msg : "(null)");
        }

        std::u8string_view getStringsSuffix(LocalizedStringType type)
        {
            switch (type)
            {
                case LocalizedStringType::Strings:
                    return u8".STRINGS";
                case LocalizedStringType::ILStrings:
                    return u8".ILSTRINGS";
                case LocalizedStringType::DLStrings:
                    return u8".DLSTRINGS";
            }

            throw std::logic_error("Unsupported LocalizedStringType: " + std::to_string(static_cast<int>(type)));
        }

        struct InflateEnd
        {
            void operator()(z_stream* stream) const { inflateEnd(stream); }
        };

        std::optional<std::string> tryDecompressAll(std::span<char> compressed, std::span<char> decompressed)
        {
            z_stream stream{};

            stream.next_in = reinterpret_cast<Bytef*>(compressed.data());
            stream.next_out = reinterpret_cast<Bytef*>(decompressed.data());
            stream.avail_in = static_cast<uInt>(compressed.size());
            stream.avail_out = static_cast<uInt>(decompressed.size());

            if (const int ec = inflateInit(&stream); ec != Z_OK)
                return getError("inflateInit error", ec, stream.msg);

            const std::unique_ptr<z_stream, InflateEnd> streamPtr(&stream);

            if (const int ec = inflate(&stream, Z_NO_FLUSH); ec != Z_STREAM_END)
                return getError("inflate error", ec, stream.msg);

            return std::nullopt;
        }

        std::optional<std::string> tryDecompressByBlock(
            std::span<char> compressed, std::span<char> decompressed, std::size_t blockSize)
        {
            z_stream stream{};

            if (const int ec = inflateInit(&stream); ec != Z_OK)
                return getError("inflateInit error", ec, stream.msg);

            const std::unique_ptr<z_stream, InflateEnd> streamPtr(&stream);

            while (!compressed.empty() && !decompressed.empty())
            {
                const auto prevTotalIn = stream.total_in;
                const auto prevTotalOut = stream.total_out;
                stream.next_in = reinterpret_cast<Bytef*>(compressed.data());
                stream.avail_in = static_cast<uInt>(std::min(blockSize, compressed.size()));
                stream.next_out = reinterpret_cast<Bytef*>(decompressed.data());
                stream.avail_out = static_cast<uInt>(std::min(blockSize, decompressed.size()));
                const int ec = inflate(&stream, Z_NO_FLUSH);
                if (ec == Z_STREAM_END)
                    break;
                if (ec != Z_OK)
                    return getError(
                        "inflate error after reading " + std::to_string(stream.total_in) + " bytes", ec, stream.msg);
                compressed = compressed.subspan(stream.total_in - prevTotalIn);
                decompressed = decompressed.subspan(stream.total_out - prevTotalOut);
            }

            return std::nullopt;
        }

        std::unique_ptr<Bsa::MemoryInputStream> decompress(
            std::streamoff position, std::span<char> compressed, std::uint32_t uncompressedSize)
        {
            auto result = std::make_unique<Bsa::MemoryInputStream>(uncompressedSize);

            const std::span decompressed(result->getRawData(), uncompressedSize);

            const auto allError = tryDecompressAll(compressed, decompressed);
            if (!allError.has_value())
                return result;

            Log(Debug::Warning) << "Failed to decompress record data at 0x" << std::hex << position
                                << std::resetiosflags(std::ios_base::hex) << " compressed size = " << compressed.size()
                                << " uncompressed size = " << uncompressedSize << ": " << *allError
                                << ". Trying to decompress by block...";

            std::memset(result->getRawData(), 0, uncompressedSize);

            constexpr std::size_t blockSize = 4;
            const auto blockError = tryDecompressByBlock(compressed, decompressed, blockSize);
            if (!blockError.has_value())
                return result;

            std::ostringstream s;
            s << "Failed to decompress record data by block of " << blockSize << " bytes at 0x" << std::hex << position
              << std::resetiosflags(std::ios_base::hex) << " compressed size = " << compressed.size()
              << " uncompressed size = " << uncompressedSize << ": " << *blockError;
            throw std::runtime_error(s.str());
        }
    }

    ReaderContext::ReaderContext()
        : modIndex(0)
        , recHeaderSize(sizeof(RecordHeader))
        , filePos(0)
        , fileRead(0)
        , recordRead(0)
        , currWorld({ 0, 0 })
        , currCell({ 0, 0 })
        , currCellGrid(FormId{ 0, 0 })
        , cellGridValid(false)
    {
        recordHeader = {};
        subRecordHeader = {};
    }

    Reader::Reader(Files::IStreamPtr&& esmStream, const std::filesystem::path& filename, VFS::Manager const* vfs,
        const ToUTF8::StatelessUtf8Encoder* encoder, bool ignoreMissingLocalizedStrings)
        : mVFS(vfs)
        , mEncoder(encoder)
        , mFileSize(0)
        , mStream(std::move(esmStream))
        , mIgnoreMissingLocalizedStrings(ignoreMissingLocalizedStrings)
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
        if (mCtx.filePos == std::streampos(-1))
            return mCtx;
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
        // clearCtx();
        // mHeader.blank();
    }

    void Reader::openRaw(Files::IStreamPtr&& stream, const std::filesystem::path& filename)
    {
        close();

        mStream = std::move(stream);
        mCtx.filename = filename;
        mCtx.fileRead = 0;
        mStream->seekg(0, mStream->end);
        mFileSize = mStream->tellg();
        mStream->seekg(0, mStream->beg);
    }

    void Reader::open(Files::IStreamPtr&& stream, const std::filesystem::path& filename)
    {
        openRaw(std::move(stream), filename);

        // should at least have the size of ESM3 record header (20 or 24 bytes for ESM4)
        if (mFileSize < 16)
            throw std::runtime_error("File too small");
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

    void Reader::open(const std::filesystem::path& filename)
    {
        open(Files::openConstrainedFileStream(filename), filename);
    }

    void Reader::setRecHeaderSize(const std::size_t size)
    {
        mCtx.recHeaderSize = size;
    }

    void Reader::buildLStringIndex()
    {
        if ((mHeader.mFlags & Rec_ESM) == 0 || (mHeader.mFlags & Rec_Localized) == 0)
            return;

        const std::u8string prefix = mCtx.filename.stem().filename().u8string();

        buildLStringIndex(LocalizedStringType::Strings, prefix);
        buildLStringIndex(LocalizedStringType::ILStrings, prefix);
        buildLStringIndex(LocalizedStringType::DLStrings, prefix);
    }

    void Reader::buildLStringIndex(LocalizedStringType stringType, const std::u8string& prefix)
    {
        static const std::filesystem::path strings("Strings");
        const std::u8string language(u8"_En");
        const std::u8string altLanguage(u8"_English");
        const std::u8string suffix(getStringsSuffix(stringType));
        std::filesystem::path path = strings / (prefix + language + suffix);
        if (mVFS != nullptr)
        {
            VFS::Path::Normalized vfsPath(Files::pathToUnicodeString(path));
            Files::IStreamPtr stream = mVFS->find(vfsPath);

            if (stream == nullptr)
            {
                path = strings / (prefix + altLanguage + suffix);
                vfsPath = VFS::Path::Normalized(Files::pathToUnicodeString(path));
                stream = mVFS->find(vfsPath);
            }

            if (stream != nullptr)
            {
                buildLStringIndex(stringType, *stream);
                return;
            }

            if (mIgnoreMissingLocalizedStrings)
            {
                Log(Debug::Warning) << "Ignore missing VFS strings file: " << vfsPath;
                return;
            }
        }

        std::filesystem::path fsPath = mCtx.filename.parent_path() / path;
        if (!std::filesystem::exists(fsPath))
        {
            path = strings / (prefix + altLanguage + suffix);
            fsPath = mCtx.filename.parent_path() / path;
        }

        if (std::filesystem::exists(fsPath))
        {
            const Files::IStreamPtr stream = Files::openConstrainedFileStream(fsPath);
            buildLStringIndex(stringType, *stream);
            return;
        }

        if (mIgnoreMissingLocalizedStrings)
            Log(Debug::Warning) << "Ignore missing strings file: " << fsPath;
    }

    void Reader::buildLStringIndex(LocalizedStringType stringType, std::istream& stream)
    {
        stream.seekg(0, std::ios::end);
        const std::istream::pos_type fileSize = stream.tellg();
        stream.seekg(0, std::ios::beg);

        std::uint32_t numEntries = 0;
        stream.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));

        std::uint32_t dataSize = 0;
        stream.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

        const std::istream::pos_type dataStart = fileSize - static_cast<std::istream::pos_type>(dataSize);

        struct LocalizedString
        {
            std::uint32_t mOffset = 0;
            std::uint32_t mStringId = 0;
        };

        std::vector<LocalizedString> strings;
        strings.reserve(numEntries);

        for (std::uint32_t i = 0; i < numEntries; ++i)
        {
            LocalizedString string;

            stream.read(reinterpret_cast<char*>(&string.mStringId), sizeof(string.mStringId));
            stream.read(reinterpret_cast<char*>(&string.mOffset), sizeof(string.mOffset));

            strings.push_back(string);
        }

        std::sort(strings.begin(), strings.end(),
            [](const LocalizedString& l, const LocalizedString& r) { return l.mOffset < r.mOffset; });

        std::uint32_t lastOffset = 0;
        std::string_view lastValue;

        for (const LocalizedString& string : strings)
        {
            if (string.mOffset == lastOffset)
            {
                mLStringIndex.emplace(FormId::fromUint32(string.mStringId), lastValue);
                continue;
            }

            const std::istream::pos_type offset = string.mOffset + dataStart;
            const std::istream::pos_type pos = stream.tellg();
            if (pos != offset)
            {
                char buffer[4096];
                if (pos < offset && offset - pos < static_cast<std::istream::pos_type>(sizeof(buffer)))
                    stream.read(buffer, offset - pos);
                else
                    stream.seekg(offset);
            }

            const auto it
                = mLStringIndex.emplace(FormId::fromUint32(string.mStringId), readLocalizedString(stringType, stream))
                      .first;
            lastOffset = string.mOffset;
            lastValue = it->second;
        }
    }

    std::string Reader::readLocalizedString(LocalizedStringType type, std::istream& stream)
    {
        if (type == LocalizedStringType::Strings)
        {
            std::string data;

            while (true)
            {
                char ch = 0;
                stream.read(&ch, sizeof(ch));
                if (ch == 0)
                    break;
                data.push_back(ch);
            }

            return data;
        }

        std::uint32_t size = 0;
        stream.read(reinterpret_cast<char*>(&size), sizeof(size));

        std::string result;
        getStringImpl(result, size, stream, true); // expect null terminated string
        return result;
    }

    void Reader::getLocalizedString(std::string& str)
    {
        if (!hasLocalizedStrings())
            return (void)getZString(str);

        std::uint32_t stringId; // FormId
        get(stringId);
        if (stringId) // TES5 FoxRace, BOOK
            getLocalizedStringImpl(FormId::fromUint32(stringId), str);
    }

    // FIXME: very messy and probably slow/inefficient
    void Reader::getLocalizedStringImpl(const FormId stringId, std::string& str)
    {
        const auto it = mLStringIndex.find(stringId);

        if (it == mLStringIndex.end())
        {
            if (mIgnoreMissingLocalizedStrings)
                return;
            throw std::runtime_error("ESM4::Reader::getLocalizedString localized string not found for "
                + ESM::RefId(stringId).toDebugString());
        }

        str = it->second;
    }

    bool Reader::getRecordHeader()
    {
        // FIXME: this seems very hacky but we may have skipped subrecords from within an inflated data block
        if (/*mStream->eof() && */ mSavedStream)
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

            const std::streamoff position = mStream->tellg();

            const std::uint32_t recordSize = mCtx.recordHeader.record.dataSize - sizeof(std::uint32_t);
            std::vector<char> compressed(recordSize);
            mStream->read(compressed.data(), recordSize);
            mSavedStream = std::move(mStream);

            mCtx.recordHeader.record.dataSize = uncompressedSize - sizeof(uncompressedSize);

            auto memoryStreamPtr = decompress(position, compressed, uncompressedSize);

            // For debugging only
            // #if 0
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
                    else if (i < uncompressedSize - 1)
                        ss << " ";
                }
                std::cout << ss.str() << std::endl;
            }
            // #endif
            mStream = std::make_unique<Files::StreamWithBuffer<Bsa::MemoryInputStream>>(std::move(memoryStreamPtr));
        }
    }

    void Reader::skipRecordData()
    {
        if (mCtx.recordRead > mCtx.recordHeader.record.dataSize)
            throw std::runtime_error("Skipping after reading more than available");
        mStream->ignore(mCtx.recordHeader.record.dataSize - mCtx.recordRead);
        mCtx.recordRead = mCtx.recordHeader.record.dataSize; // for getSubRecordHeader()
    }

    bool Reader::getSubRecordHeader()
    {
        bool result = false;
        // NOTE: some SubRecords have 0 dataSize (e.g. SUB_RDSD in one of REC_REGN records in Oblivion.esm).
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

        // Extended storage subrecord redefines the following subrecord's size.
        // Would need to redesign the loader to support that, so skip over both subrecords.
        if (result && mCtx.subRecordHeader.typeId == ESM::fourCC("XXXX"))
        {
            std::uint32_t extDataSize;
            get(extDataSize);
            if (!getSubRecordHeader())
                return false;

            skipSubRecordData(extDataSize);
            mCtx.recordRead += extDataSize - mCtx.subRecordHeader.dataSize;
            return getSubRecordHeader();
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
        padding.insert(0, mCtx.groupStack.size() * 2, ' ');
        std::cout << padding << "Starting record group "
                  << printLabel(mCtx.recordHeader.group.label, mCtx.recordHeader.group.type) << std::endl;
#endif
        // empty group if the group size is same as the header size
        if (mCtx.recordHeader.group.groupSize == (std::uint32_t)mCtx.recHeaderSize)
        {
#ifdef DEBUG_GROUPSTACK
            std::cout << padding << "Ignoring record group " // FIXME: debugging only
                      << printLabel(mCtx.recordHeader.group.label, mCtx.recordHeader.group.type) << " (empty)"
                      << std::endl;
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
            padding.insert(0, mCtx.groupStack.size() * 2, ' ');
            std::cout << padding << "Finished record group " << printLabel(grp.label, grp.type) << std::endl;
#endif
            // if the previous group was the final one no need to do below
            if (mCtx.groupStack.empty())
                return;

            mCtx.groupStack.back().second += lastGroupSize;
            lastGroupSize = mCtx.groupStack.back().first.groupSize;

            if (lastGroupSize < mCtx.groupStack.back().second)
                throw std::runtime_error("Read more records than available");

            // #if 0
            if (mCtx.groupStack.back().second > lastGroupSize) // FIXME: debugging only
                std::cerr << printLabel(mCtx.groupStack.back().first.label, mCtx.groupStack.back().first.type)
                          << " read more records than available" << std::endl;
            // #endif
        }
    }

    // WARNING: this method should be used after first calling enterGroup()
    // else the method may try to dereference an element that does not exist
    const GroupTypeHeader& Reader::grp(std::size_t pos) const
    {
        if (mCtx.groupStack.size() == 0)
            throw std::runtime_error("ESM4::Reader::grp mCtx.groupStack.size is zero");
        if (pos > mCtx.groupStack.size() - 1)
            throw std::runtime_error("ESM4::Reader::grp - exceeded stack depth");

        return (*(mCtx.groupStack.end() - pos - 1)).first;
    }

    void Reader::skipGroupData()
    {
        if (mCtx.groupStack.empty())
            throw std::runtime_error("Skipping group with an empty stack");

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
        padding.insert(0, mCtx.groupStack.size() * 2, ' ');
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
        if (!mCtx.cellGridValid)
            throw std::runtime_error("Attempt to use an invalid cell grid");

        return mCtx.currCellGrid;
    }

    void Reader::updateModIndices(const std::map<std::string, int>& fileToModIndex)
    {
        mCtx.parentFileIndices.resize(mHeader.mMaster.size());
        for (unsigned int i = 0; i < mHeader.mMaster.size(); ++i)
        {
            // locate the position of the dependency in already loaded files
            auto it = fileToModIndex.find(Misc::StringUtils::lowerCase(mHeader.mMaster[i].name));
            if (it != fileToModIndex.end())
                mCtx.parentFileIndices[i] = it->second;
            else
                throw std::runtime_error(
                    "ESM4::Reader::updateModIndices required dependency '" + mHeader.mMaster[i].name + "' not found");
        }
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
    void Reader::adjustFormId(FormId& id) const
    {
        if (id.hasContentFile() && id.mContentFile < static_cast<int>(mCtx.parentFileIndices.size()))
            id.mContentFile = mCtx.parentFileIndices[id.mContentFile];
        else
            id.mContentFile = mCtx.modIndex;
    }

    void Reader::adjustFormId(FormId32& id) const
    {
        FormId formId = FormId::fromUint32(id);
        adjustFormId(formId);
        id = formId.toUint32();
    }

    bool Reader::getFormId(FormId& id)
    {
        FormId32 v;
        if (!getExact(v))
            return false;
        id = FormId::fromUint32(v);

        adjustFormId(id);
        return true;
    }

    ESM::FormId Reader::getFormIdFromHeader() const
    {
        FormId formId = hdr().record.getFormId();
        adjustFormId(formId);
        return formId;
    }

    void Reader::adjustGRUPFormId()
    {
        adjustFormId(mCtx.recordHeader.group.label.value);
    }

    [[noreturn]] void Reader::fail(const std::string& msg)
    {
        std::stringstream ss;

        ss << "ESM Error: " << msg;
        ss << "\n  File: " << Files::pathToUnicodeString(mCtx.filename);
        ss << "\n  Record: " << ESM::printName(mCtx.recordHeader.record.typeId);
        ss << "\n  Subrecord: " << ESM::printName(mCtx.subRecordHeader.typeId);
        if (mStream.get())
            ss << "\n  Offset: 0x" << std::hex << mStream->tellg();

        throw std::runtime_error(ss.str());
    }

    bool Reader::getStringImpl(std::string& str, std::size_t size, std::istream& stream, bool hasNull)
    {
        std::size_t newSize = size;

        if (mEncoder != nullptr)
        {
            std::string input(size, '\0');
            stream.read(input.data(), size);
            if (stream.gcount() == static_cast<std::streamsize>(size))
            {
                const std::string_view result
                    = mEncoder->getUtf8(input, ToUTF8::BufferAllocationPolicy::FitToRequiredSize, str);
                if (str.empty() && !result.empty())
                {
                    str = std::move(input);
                    str.resize(result.size());
                }
                return true;
            }
        }
        else
        {
            if (hasNull)
                newSize -= 1; // don't read the null terminator yet

            str.resize(newSize); // assumed C++11
            stream.read(str.data(), newSize);
            if (static_cast<std::size_t>(stream.gcount()) == newSize)
            {
                if (hasNull)
                {
                    char ch;
                    stream.read(&ch, 1); // read the null terminator
                    if (ch != '\0')
                        throw std::runtime_error("ESM4::Reader::getString string is not terminated with a null");
                }
#if 0
            else
            {
                // NOTE: normal ESMs don't but omwsave has locals or spells with null terminator
                assert (str[newSize - 1] != '\0'
                        && "ESM4::Reader::getString string is unexpectedly terminated with a null");
            }
#endif
                return true;
            }
        }

        str.clear();
        return false; // FIXME: throw instead?
    }

    bool Reader::getZeroTerminatedStringArray(std::vector<std::string>& values)
    {
        const std::size_t size = mCtx.subRecordHeader.dataSize;
        std::string input(size, '\0');
        mStream->read(input.data(), size);

        if (mStream->gcount() != static_cast<std::streamsize>(size))
            return false;

        std::string_view inputView(input.data(), input.size());
        std::string buffer;
        while (true)
        {
            std::string_view value(inputView.data());
            const std::size_t next = inputView.find_first_not_of('\0', value.size());
            if (mEncoder != nullptr)
                value = mEncoder->getUtf8(value, ToUTF8::BufferAllocationPolicy::UseGrowFactor, buffer);
            values.emplace_back(value);
            if (next == std::string_view::npos)
                break;
            inputView = inputView.substr(next);
        }

        return true;
    }

    namespace
    {
        constexpr std::string_view sGroupType[] = {
            "Record Type",
            "World Child",
            "Interior Cell",
            "Interior Sub Cell",
            "Exterior Cell",
            "Exterior Sub Cell",
            "Cell Child",
            "Topic Child",
            "Cell Persistent Child",
            "Cell Temporary Child",
            "Cell Visible Dist Child",
            "Unknown",
        };
    }

    std::string printLabel(const GroupLabel& label, const std::uint32_t type)
    {
        std::ostringstream ss;
        ss << sGroupType[std::min<std::size_t>(type, std::size(sGroupType))]; // avoid out of range

        switch (type)
        {
            case ESM4::Grp_RecordType:
            {
                ss << ": " << std::string((char*)label.recordType, 4);
                break;
            }
            case ESM4::Grp_ExteriorCell:
            case ESM4::Grp_ExteriorSubCell:
            {
                // short x, y;
                // y = label & 0xff;
                // x = (label >> 16) & 0xff;
                ss << ": grid (x, y) " << std::dec << label.grid[1] << ", " << label.grid[0];

                break;
            }
            case ESM4::Grp_InteriorCell:
            case ESM4::Grp_InteriorSubCell:
            {
                ss << ": block 0x" << std::hex << label.value;
                break;
            }
            case ESM4::Grp_WorldChild:
            case ESM4::Grp_CellChild:
            case ESM4::Grp_TopicChild:
            case ESM4::Grp_CellPersistentChild:
            case ESM4::Grp_CellTemporaryChild:
            case ESM4::Grp_CellVisibleDistChild:
            {
                ss << ": " << ESM::RefId(FormId::fromUint32(label.value));
                break;
            }
            default:
                break;
        }

        return ss.str();
    }
}
