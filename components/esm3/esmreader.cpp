#include "esmreader.hpp"

#include "readerscache.hpp"

#include <components/esm3/cellid.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/files/conversion.hpp>
#include <components/files/openfile.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ESM
{

    ESM_Context ESMReader::getContext()
    {
        // Update the file position before returning
        mCtx.filePos = mEsm->tellg();
        return mCtx;
    }

    ESMReader::ESMReader()
        : mRecordFlags(0)
        , mBuffer(50 * 1024)
        , mEncoder(nullptr)
        , mFileSize(0)
    {
        clearCtx();
        mCtx.index = 0;
    }

    void ESMReader::restoreContext(const ESM_Context& rc)
    {
        // Reopen the file if necessary
        if (mCtx.filename != rc.filename)
            openRaw(rc.filename);

        // Copy the data
        mCtx = rc;

        // Make sure we seek to the right place
        mEsm->seekg(mCtx.filePos);
    }

    void ESMReader::close()
    {
        mEsm.reset();
        clearCtx();
        mHeader.blank();
    }

    void ESMReader::clearCtx()
    {
        mCtx.filename.clear();
        mCtx.leftFile = 0;
        mCtx.leftRec = 0;
        mCtx.leftSub = 0;
        mCtx.subCached = false;
        mCtx.recName.clear();
        mCtx.subName.clear();
    }

    void ESMReader::resolveParentFileIndices(ReadersCache& readers)
    {
        mCtx.parentFileIndices.clear();
        for (const Header::MasterData& mast : getGameFiles())
        {
            const std::string& fname = mast.name;
            int index = getIndex();
            for (int i = 0; i < getIndex(); i++)
            {
                const ESM::ReadersCache::BusyItem reader = readers.get(static_cast<std::size_t>(i));
                if (reader->getFileSize() == 0)
                    continue; // Content file in non-ESM format
                const auto fnamecandidate = Files::pathToUnicodeString(reader->getName().filename());
                if (Misc::StringUtils::ciEqual(fname, fnamecandidate))
                {
                    index = i;
                    break;
                }
            }
            mCtx.parentFileIndices.push_back(index);
        }
    }

    bool ESMReader::applyContentFileMapping(FormId& id)
    {
        if (mContentFileMapping && id.hasContentFile())
        {
            auto iter = mContentFileMapping->find(id.mContentFile);
            if (iter == mContentFileMapping->end())
                return false;
            id.mContentFile = iter->second;
        }
        return true;
    }

    ESM::RefId ESMReader::getCellId()
    {
        if (mHeader.mFormatVersion <= ESM::MaxUseEsmCellIdFormatVersion)
        {
            ESM::CellId cellId;
            cellId.load(*this);
            if (cellId.mPaged)
            {
                return ESM::RefId::esm3ExteriorCell(cellId.mIndex.mX, cellId.mIndex.mY);
            }
            else
            {
                return ESM::RefId::stringRefId(cellId.mWorldspace);
            }
        }
        return getHNRefId("NAME");
    }

    void ESMReader::openRaw(std::unique_ptr<std::istream>&& stream, const std::filesystem::path& name)
    {
        close();
        mEsm = std::move(stream);
        mCtx.filename = name;
        mEsm->seekg(0, mEsm->end);
        mCtx.leftFile = mFileSize = mEsm->tellg();
        mEsm->seekg(0, mEsm->beg);
    }

    void ESMReader::openRaw(const std::filesystem::path& filename)
    {
        openRaw(Files::openBinaryInputFileStream(filename), filename);
    }

    void ESMReader::open(std::unique_ptr<std::istream>&& stream, const std::filesystem::path& name)
    {
        openRaw(std::move(stream), name);

        if (getRecName() != "TES3")
            fail("Not a valid Morrowind file");

        getRecHeader();

        mHeader.load(*this);
    }

    void ESMReader::open(const std::filesystem::path& file)
    {
        open(Files::openBinaryInputFileStream(file), file);
    }

    std::string ESMReader::getHNOString(NAME name)
    {
        if (isNextSub(name))
            return getHString();
        return {};
    }

    ESM::RefId ESMReader::getHNORefId(NAME name)
    {
        if (isNextSub(name))
            return getRefId();
        return ESM::RefId();
    }

    void ESMReader::skipHNORefId(NAME name)
    {
        if (isNextSub(name))
            skipHRefId();
    }

    std::string ESMReader::getHNString(NAME name)
    {
        getSubNameIs(name);
        return getHString();
    }

    RefId ESMReader::getHNRefId(NAME name)
    {
        getSubNameIs(name);
        return getRefId();
    }

    std::string ESMReader::getHString()
    {
        return std::string(getHStringView());
    }

    std::string_view ESMReader::getHStringView()
    {
        getSubHeader();

        if (mHeader.mFormatVersion > MaxStringRefIdFormatVersion)
            return getStringView(mCtx.leftSub);

        // Hack to make MultiMark.esp load. Zero-length strings do not
        // occur in any of the official mods, but MultiMark makes use of
        // them. For some reason, they break the rules, and contain a byte
        // (value 0) even if the header says there is no data. If
        // Morrowind accepts it, so should we.
        if (mCtx.leftSub == 0 && hasMoreSubs() && !mEsm->peek())
        {
            // Skip the following zero byte
            mCtx.leftRec--;
            char c;
            getT(c);
            return std::string_view();
        }

        return getStringView(mCtx.leftSub);
    }

    RefId ESMReader::getRefId()
    {
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion)
            return ESM::RefId::stringRefId(getHStringView());
        getSubHeader();
        return getRefIdImpl(mCtx.leftSub);
    }

    void ESMReader::skipHString()
    {
        getSubHeader();

        // Hack to make MultiMark.esp load. Zero-length strings do not
        // occur in any of the official mods, but MultiMark makes use of
        // them. For some reason, they break the rules, and contain a byte
        // (value 0) even if the header says there is no data. If
        // Morrowind accepts it, so should we.
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion && mCtx.leftSub == 0 && hasMoreSubs()
            && !mEsm->peek())
        {
            // Skip the following zero byte
            mCtx.leftRec--;
            skipT<char>();
            return;
        }

        skip(mCtx.leftSub);
    }

    void ESMReader::skipHRefId()
    {
        skipHString();
    }

    FormId ESMReader::getFormId(bool wide, NAME tag)
    {
        FormId res;
        if (wide)
            getHNT(tag, res.mIndex, res.mContentFile);
        else
            getHNT(res.mIndex, tag);
        return res;
    }

    // Get the next subrecord name and check if it matches the parameter
    void ESMReader::getSubNameIs(NAME name)
    {
        getSubName();
        if (mCtx.subName != name)
            fail("Expected subrecord " + name.toString() + " but got " + mCtx.subName.toString());
    }

    bool ESMReader::isNextSub(NAME name)
    {
        if (!hasMoreSubs())
            return false;

        getSubName();

        // If the name didn't match, then mark the it as 'cached' so it's
        // available for the next call to getSubName.
        mCtx.subCached = (mCtx.subName != name);

        // If subCached is false, then subName == name.
        return !mCtx.subCached;
    }

    bool ESMReader::peekNextSub(NAME name)
    {
        if (!hasMoreSubs())
            return false;

        getSubName();

        mCtx.subCached = true;
        return mCtx.subName == name;
    }

    // Read subrecord name. This gets called a LOT, so I've optimized it
    // slightly.
    void ESMReader::getSubName()
    {
        // If the name has already been read, do nothing
        if (mCtx.subCached)
        {
            mCtx.subCached = false;
            return;
        }

        // reading the subrecord data anyway.
        const std::size_t subNameSize = decltype(mCtx.subName)::sCapacity;
        getExact(mCtx.subName.mData, subNameSize);
        mCtx.leftRec -= static_cast<std::uint32_t>(subNameSize);
    }

    void ESMReader::skipHSub()
    {
        getSubHeader();
        skip(mCtx.leftSub);
    }

    void ESMReader::skipHSubSize(std::size_t size)
    {
        skipHSub();
        if (mCtx.leftSub != size)
            reportSubSizeMismatch(mCtx.leftSub, size);
    }

    void ESMReader::skipHSubUntil(NAME name)
    {
        while (hasMoreSubs() && !isNextSub(name))
        {
            mCtx.subCached = false;
            skipHSub();
        }
        if (hasMoreSubs())
            mCtx.subCached = true;
    }

    void ESMReader::getSubHeader()
    {
        if (mCtx.leftRec < static_cast<std::streamsize>(sizeof(mCtx.leftSub)))
            fail("End of record while reading sub-record header: " + std::to_string(mCtx.leftRec) + " < "
                + std::to_string(sizeof(mCtx.leftSub)));

        // Get subrecord size
        getUint(mCtx.leftSub);
        mCtx.leftRec -= sizeof(mCtx.leftSub);

        // Adjust number of record bytes left; may go negative
        mCtx.leftRec -= mCtx.leftSub;
    }

    NAME ESMReader::getRecName()
    {
        if (!hasMoreRecs())
            fail("No more records, getRecName() failed");
        if (hasMoreSubs())
            fail("Previous record contains unread bytes");

        // We went out of the previous record's bounds. Backtrack.
        if (mCtx.leftRec < 0)
            mEsm->seekg(mCtx.leftRec, std::ios::cur);

        getName(mCtx.recName);
        mCtx.leftFile -= decltype(mCtx.recName)::sCapacity;

        // Make sure we don't carry over any old cached subrecord
        // names. This can happen in some cases when we skip parts of a
        // record.
        mCtx.subCached = false;

        return mCtx.recName;
    }

    void ESMReader::skipRecord()
    {
        skip(mCtx.leftRec);
        mCtx.leftRec = 0;
        mCtx.subCached = false;
    }

    void ESMReader::getRecHeader(uint32_t& flags)
    {
        if (mCtx.leftFile < static_cast<std::streamsize>(3 * sizeof(uint32_t)))
            fail("End of file while reading record header");

        std::uint32_t leftRec = 0;
        getUint(leftRec);
        mCtx.leftRec = static_cast<std::streamsize>(leftRec);
        getUint(flags); // This header entry is always zero
        getUint(flags);
        mCtx.leftFile -= 3 * sizeof(uint32_t);

        // Check that sizes add up
        if (mCtx.leftFile < mCtx.leftRec)
            reportSubSizeMismatch(mCtx.leftFile, mCtx.leftRec);

        // Adjust number of bytes mCtx.left in file
        mCtx.leftFile -= mCtx.leftRec;
    }

    /*************************************************************************
     *
     *  Lowest level data reading and misc methods
     *
     *************************************************************************/

    std::string ESMReader::getMaybeFixedStringSize(std::size_t size)
    {
        if (mHeader.mFormatVersion > MaxLimitedSizeStringsFormatVersion)
        {
            StringSizeType storedSize = 0;
            getT(storedSize);
            if (storedSize > mCtx.leftSub)
                fail("String does not fit subrecord (" + std::to_string(storedSize) + " > "
                    + std::to_string(mCtx.leftSub) + ")");
            size = static_cast<std::size_t>(storedSize);
        }

        return std::string(getStringView(size));
    }

    RefId ESMReader::getMaybeFixedRefIdSize(std::size_t size)
    {
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion)
            return RefId::stringRefId(getMaybeFixedStringSize(size));
        return getRefIdImpl(mCtx.leftSub);
    }

    std::string_view ESMReader::getStringView(std::size_t size)
    {
        if (mBuffer.size() <= size)
            // Add some extra padding to reduce the chance of having to resize
            // again later.
            mBuffer.resize(3 * size);

        // And make sure the string is zero terminated
        mBuffer[size] = 0;

        // read ESM data
        char* ptr = mBuffer.data();
        getExact(ptr, size);

        size = strnlen(ptr, size);

        // Convert to UTF8 and return
        if (mEncoder != nullptr)
            return mEncoder->getUtf8(std::string_view(ptr, size));

        return std::string_view(ptr, size);
    }

    RefId ESMReader::getRefId(std::size_t size)
    {
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion)
            return ESM::RefId::stringRefId(getStringView(size));
        return getRefIdImpl(size);
    }

    RefId ESMReader::getRefIdImpl(std::size_t size)
    {
        RefIdType refIdType = RefIdType::Empty;
        getT(refIdType);

        switch (refIdType)
        {
            case RefIdType::Empty:
                return RefId();
            case RefIdType::SizedString:
            {
                const std::size_t minSize = sizeof(refIdType) + sizeof(StringSizeType);
                if (size < minSize)
                    fail("Requested RefId record size is too small (" + std::to_string(size) + " < "
                        + std::to_string(minSize) + ")");
                StringSizeType storedSize = 0;
                getT(storedSize);
                const std::size_t maxSize = size - minSize;
                if (storedSize > maxSize)
                    fail("RefId string does not fit subrecord size (" + std::to_string(storedSize) + " > "
                        + std::to_string(maxSize) + ")");
                return RefId::stringRefId(getStringView(storedSize));
            }
            case RefIdType::UnsizedString:
                if (size < sizeof(refIdType))
                    fail("Requested RefId record size is too small (" + std::to_string(size) + " < "
                        + std::to_string(sizeof(refIdType)) + ")");
                return RefId::stringRefId(getStringView(size - sizeof(refIdType)));
            case RefIdType::FormId:
            {
                FormId formId{};
                getT(formId.mIndex);
                getT(formId.mContentFile);
                if (applyContentFileMapping(formId))
                    return RefId(formId);
                else
                    return RefId(); // content file was removed from load order
            }
            case RefIdType::Generated:
            {
                std::uint64_t generated{};
                getT(generated);
                return RefId::generated(generated);
            }
            case RefIdType::Index:
            {
                RecNameInts recordType{};
                getExact(&recordType, sizeof(std::uint32_t));
                std::uint32_t index{};
                getT(index);
                return RefId::index(recordType, index);
            }
            case RefIdType::ESM3ExteriorCell:
            {
                int32_t x, y;
                getT(x);
                getT(y);
                return RefId::esm3ExteriorCell(x, y);
            }
        }

        fail("Unsupported RefIdType: " + std::to_string(static_cast<unsigned>(refIdType)));
    }

    [[noreturn]] void ESMReader::fail(std::string_view msg)
    {
        std::stringstream ss;

        ss << "ESM Error: " << msg;
        ss << "\n  File: " << Files::pathToUnicodeString(mCtx.filename);
        ss << "\n  Record: " << mCtx.recName.toStringView();
        ss << "\n  Subrecord: " << mCtx.subName.toStringView();
        if (mEsm.get())
            ss << "\n  Offset: 0x" << std::hex << mEsm->tellg();
        throw std::runtime_error(ss.str());
    }

}
