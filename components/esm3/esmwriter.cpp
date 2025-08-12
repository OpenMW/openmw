#include "esmwriter.hpp"

#include <cassert>
#include <fstream>
#include <stdexcept>

#include <components/debug/debuglog.hpp>
#include <components/esm3/cellid.hpp>
#include <components/misc/notnullptr.hpp>
#include <components/toutf8/toutf8.hpp>

#include "formatversion.hpp"

namespace ESM
{
    namespace
    {
        template <bool sizedString>
        struct WriteRefId
        {
            ESMWriter& mWriter;

            explicit WriteRefId(ESMWriter& writer)
                : mWriter(writer)
            {
            }

            void operator()(EmptyRefId /*v*/) const { mWriter.writeT(RefIdType::Empty); }

            void operator()(StringRefId v) const
            {
                constexpr StringSizeType maxSize = std::numeric_limits<StringSizeType>::max();
                if (v.getValue().size() > maxSize)
                    throw std::runtime_error("RefId string size is too long: \"" + v.getValue().substr(0, 64)
                        + "<...>\" (" + std::to_string(v.getValue().size()) + " > " + std::to_string(maxSize) + ")");
                if constexpr (sizedString)
                {
                    mWriter.writeT(RefIdType::SizedString);
                    mWriter.writeT(static_cast<StringSizeType>(v.getValue().size()));
                }
                else
                    mWriter.writeT(RefIdType::UnsizedString);
                mWriter.write(v.getValue().data(), v.getValue().size());
            }

            void operator()(FormId v) const
            {
                mWriter.writeT(RefIdType::FormId);
                mWriter.writeT(v);
            }

            void operator()(GeneratedRefId v) const
            {
                mWriter.writeT(RefIdType::Generated);
                mWriter.writeT(v.getValue());
            }

            void operator()(IndexRefId v) const
            {
                mWriter.writeT(RefIdType::Index);
                mWriter.writeT(v.getRecordType());
                mWriter.writeT(v.getValue());
            }

            void operator()(ESM3ExteriorCellRefId v) const
            {
                mWriter.writeT(RefIdType::ESM3ExteriorCell);
                mWriter.writeT(v.getX());
                mWriter.writeT(v.getY());
            }
        };
    }

    ESMWriter::ESMWriter()
        : mRecords()
        , mStream(nullptr)
        , mHeaderPos()
        , mEncoder(nullptr)
        , mRecordCount(0)
        , mCounting(true)
        , mHeader()
    {
    }

    unsigned int ESMWriter::getVersion() const
    {
        return mHeader.mData.version.ui;
    }

    void ESMWriter::setVersion(unsigned int ver)
    {
        mHeader.mData.version.ui = ver;
    }

    void ESMWriter::setType(int type)
    {
        mHeader.mData.type = type;
    }

    void ESMWriter::setAuthor(std::string_view auth)
    {
        mHeader.mData.author = auth;
    }

    void ESMWriter::setDescription(std::string_view desc)
    {
        mHeader.mData.desc = desc;
    }

    void ESMWriter::setRecordCount(int count)
    {
        mHeader.mData.records = count;
    }

    void ESMWriter::setFormatVersion(FormatVersion value)
    {
        mHeader.mFormatVersion = value;
    }

    void ESMWriter::clearMaster()
    {
        mHeader.mMaster.clear();
    }

    void ESMWriter::addMaster(std::string_view name, uint64_t size)
    {
        Header::MasterData d;
        d.name = name;
        d.size = size;
        mHeader.mMaster.push_back(std::move(d));
    }

    void ESMWriter::save(std::ostream& file)
    {
        mRecordCount = 0;
        mRecords.clear();
        mCounting = true;
        mStream = &file;

        startRecord("TES3", 0);

        mHeader.save(*this);

        endRecord("TES3");
    }

    void ESMWriter::close()
    {
        if (!mRecords.empty())
            throw std::runtime_error("Unclosed record remaining");
    }

    void ESMWriter::startRecord(NAME name, uint32_t flags)
    {
        mRecordCount++;

        writeName(name);
        RecordData rec;
        rec.name = name;
        rec.position = mStream->tellp();
        rec.size = 0;
        writeT<uint32_t>(0); // Size goes here
        writeT<uint32_t>(0); // Unused header?
        writeT(flags);
        mRecords.push_back(rec);

        assert(mRecords.back().size == 0);
    }

    void ESMWriter::startRecord(uint32_t name, uint32_t flags)
    {
        startRecord(NAME(name), flags);
    }

    void ESMWriter::startSubRecord(NAME name)
    {
        // Sub-record hierarchies are not properly supported in ESMReader. This should be fixed later.
        assert(mRecords.size() <= 1);

        writeName(name);
        RecordData rec;
        rec.name = name;
        rec.position = mStream->tellp();
        rec.size = 0;
        writeT<uint32_t>(0); // Size goes here
        mRecords.push_back(rec);

        assert(mRecords.back().size == 0);
    }

    void ESMWriter::endRecord(NAME name)
    {
        RecordData rec = mRecords.back();
        assert(rec.name == name);
        mRecords.pop_back();

        mStream->seekp(rec.position);

        mCounting = false;
        write(reinterpret_cast<const char*>(&rec.size), sizeof(uint32_t));
        mCounting = true;

        mStream->seekp(0, std::ios::end);
    }

    void ESMWriter::endRecord(uint32_t name)
    {
        endRecord(NAME(name));
    }

    void ESMWriter::writeHNString(NAME name, std::string_view data)
    {
        startSubRecord(name);
        writeHString(data);
        endRecord(name);
    }

    void ESMWriter::writeHNString(NAME name, std::string_view data, size_t size)
    {
        assert(data.size() <= size);
        startSubRecord(name);
        writeHString(data);

        if (data.size() < size)
        {
            for (size_t i = data.size(); i < size; ++i)
                write("\0", 1);
        }

        endRecord(name);
    }

    void ESMWriter::writeHNRefId(NAME name, RefId value)
    {
        startSubRecord(name);
        writeHRefId(value);
        endRecord(name);
    }

    void ESMWriter::writeHNRefId(NAME name, RefId value, std::size_t size)
    {
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion)
            return writeHNString(name, value.getRefIdString(), size);
        writeHNRefId(name, value);
    }

    void ESMWriter::writeCellId(const ESM::RefId& cellId)
    {
        if (mHeader.mFormatVersion <= ESM::MaxUseEsmCellIdFormatVersion)
        {
            ESM::CellId generatedCellid = ESM::CellId::extractFromRefId(cellId);
            generatedCellid.save(*this);
        }
        else
            writeHNRefId("NAME", cellId);
    }

    void ESMWriter::writeMaybeFixedSizeString(const std::string& data, std::size_t size)
    {
        std::string string;
        if (!data.empty())
            string = mEncoder ? mEncoder->getLegacyEnc(data) : data;
        if (mHeader.mFormatVersion <= MaxLimitedSizeStringsFormatVersion)
        {
            if (string.size() > size)
                throw std::runtime_error("Fixed string data is too long: \"" + string + "\" ("
                    + std::to_string(string.size()) + " > " + std::to_string(size) + ")");
            string.resize(size);
        }
        else
        {
            constexpr StringSizeType maxSize = std::numeric_limits<StringSizeType>::max();
            if (string.size() > maxSize)
                throw std::runtime_error("String size is too long: \"" + string.substr(0, 64) + "<...>\" ("
                    + std::to_string(string.size()) + " > " + std::to_string(maxSize) + ")");
            writeT(static_cast<StringSizeType>(string.size()));
        }
        write(string.c_str(), string.size());
    }

    void ESMWriter::writeHString(std::string_view data)
    {
        if (data.empty())
            write("\0", 1);
        else
        {
            // Convert to UTF8 and return
            const std::string_view string = mEncoder != nullptr ? mEncoder->getLegacyEnc(data) : data;

            write(string.data(), string.size());
        }
    }

    void ESMWriter::writeHCString(std::string_view data)
    {
        writeHString(data);
        if (data.size() > 0 && data[data.size() - 1] != '\0')
            write("\0", 1);
    }

    void ESMWriter::writeMaybeFixedSizeRefId(RefId value, std::size_t size)
    {
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion)
            return writeMaybeFixedSizeString(value.getRefIdString(), size);
        visit(WriteRefId<true>(*this), value);
    }

    void ESMWriter::writeHRefId(RefId value)
    {
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion)
            return writeHString(value.getRefIdString());
        writeRefId(value);
    }

    void ESMWriter::writeHCRefId(RefId value)
    {
        if (mHeader.mFormatVersion <= MaxStringRefIdFormatVersion)
            return writeHCString(value.getRefIdString());
        writeRefId(value);
    }

    void ESMWriter::writeRefId(RefId value)
    {
        visit(WriteRefId<false>(*this), value);
    }

    void ESMWriter::writeName(NAME name)
    {
        write(name.mData, NAME::sCapacity);
    }

    void ESMWriter::write(const char* data, size_t size)
    {
        if (mCounting && !mRecords.empty())
        {
            for (std::list<RecordData>::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
                it->size += static_cast<uint32_t>(size);
        }

        mStream->write(data, size);
    }

    void ESMWriter::writeFormId(const FormId& formId, bool wide, NAME tag)
    {
        if (wide)
            writeHNT(tag, formId, 8);
        else
            writeHNT(tag, formId.toUint32(), 4);
    }

    void ESMWriter::setEncoder(ToUTF8::Utf8Encoder* encoder)
    {
        mEncoder = encoder;
    }
}
