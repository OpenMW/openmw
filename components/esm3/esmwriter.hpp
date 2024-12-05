#ifndef OPENMW_ESM_WRITER_H
#define OPENMW_ESM_WRITER_H

#include <iosfwd>
#include <list>
#include <type_traits>

#include "components/esm/decompose.hpp"
#include "components/esm/esmcommon.hpp"
#include "components/esm/refid.hpp"

#include "loadtes3.hpp"

namespace ToUTF8
{
    class Utf8Encoder;
}

namespace ESM
{

    class ESMWriter
    {
        struct RecordData
        {
            NAME name;
            std::streampos position;
            uint32_t size;
        };

    public:
        ESMWriter();

        unsigned int getVersion() const;

        // Set various header data (Header::Data). All of the below functions must be called before writing,
        // otherwise this data will be left uninitialized.
        void setVersion(unsigned int ver = 0x3fa66666);
        void setType(int type);
        void setEncoder(ToUTF8::Utf8Encoder* encoding);
        void setAuthor(std::string_view author);
        void setDescription(std::string_view desc);
        void setHeader(const Header& value) { mHeader = value; }

        // Set the record count for writing it in the file header
        void setRecordCount(int count);
        // Counts how many records we have actually written.
        // It is a good idea to compare this with the value you wrote into the header (setRecordCount)
        // It should be the record count you set + 1 (1 additional record for the TES3 header)
        int getRecordCount() const { return mRecordCount; }

        FormatVersion getFormatVersion() const { return mHeader.mFormatVersion; }
        void setFormatVersion(FormatVersion value);

        void clearMaster();

        void addMaster(std::string_view name, uint64_t size);

        void save(std::ostream& file);
        ///< Start saving a file by writing the TES3 header.

        void close();
        ///< \note Does not close the stream.

        void writeHNString(NAME name, std::string_view data);
        void writeHNString(NAME name, std::string_view data, size_t size);
        void writeHNCString(NAME name, std::string_view data)
        {
            startSubRecord(name);
            writeHCString(data);
            endRecord(name);
        }
        void writeHNOString(NAME name, std::string_view data)
        {
            if (!data.empty())
                writeHNString(name, data);
        }
        void writeHNOCString(NAME name, std::string_view data)
        {
            if (!data.empty())
                writeHNCString(name, data);
        }

        void writeHNRefId(NAME name, RefId value);

        void writeHNRefId(NAME name, RefId value, std::size_t size);

        void writeHNCRefId(NAME name, RefId value)
        {
            startSubRecord(name);
            writeHCRefId(value);
            endRecord(name);
        }

        void writeHNORefId(NAME name, RefId value)
        {
            if (!value.empty())
                writeHNRefId(name, value);
        }

        void writeHNOCRefId(NAME name, RefId value)
        {
            if (!value.empty())
                writeHNCRefId(name, value);
        }

        void writeCellId(const ESM::RefId& cellId);

        template <typename T>
        void writeHNT(NAME name, const T& data)
        {
            startSubRecord(name);
            writeT(data);
            endRecord(name);
        }

        template <typename T, std::size_t size>
        void writeHNT(NAME name, const T (&data)[size])
        {
            startSubRecord(name);
            writeT(data);
            endRecord(name);
        }

        void writeNamedComposite(NAME name, const auto& value)
        {
            decompose(value, [&](const auto&... args) {
                startSubRecord(name);
                (writeT(args), ...);
                endRecord(name);
            });
        }

        void writeComposite(const auto& value)
        {
            decompose(value, [&](const auto&... args) { (writeT(args), ...); });
        }

        // Prevent using writeHNT with strings. This already happened by accident and results in
        // state being discarded without any error on writing or reading it. :(
        // writeHNString and friends must be used instead.
        void writeHNT(NAME name, const std::string& data) = delete;
        void writeHNT(NAME name, std::string_view data) = delete;

        void writeT(NAME data) = delete;

        template <typename T, std::size_t size>
        void writeHNT(NAME name, const T (&data)[size], int) = delete;

        template <typename T>
        void writeHNT(NAME name, const T& data, std::size_t size)
        {
            startSubRecord(name);
            writeT(data, size);
            endRecord(name);
        }

        template <typename T>
        void writeT(const T& data)
        {
            static_assert(!std::is_pointer_v<T>);
            write(reinterpret_cast<const char*>(&data), sizeof(T));
        }

        template <typename T, std::size_t size>
        void writeT(const T (&data)[size])
        {
            write(reinterpret_cast<const char*>(data), size * sizeof(T));
        }

        template <typename T>
        void writeT(const T& data, size_t size)
        {
            static_assert(!std::is_pointer_v<T>);
            write((char*)&data, size);
        }

        void startRecord(NAME name, uint32_t flags = 0);
        void startRecord(uint32_t name, uint32_t flags = 0);
        /// @note Sub-record hierarchies are not properly supported in ESMReader. This should be fixed later.
        void startSubRecord(NAME name);
        void endRecord(NAME name);
        void endRecord(uint32_t name);
        void writeMaybeFixedSizeString(const std::string& data, std::size_t size);
        void writeHString(std::string_view data);
        void writeHCString(std::string_view data);

        void writeMaybeFixedSizeRefId(RefId value, std::size_t size);

        void writeHRefId(RefId refId);

        void writeHCRefId(RefId refId);

        void writeName(NAME data);

        void write(const char* data, size_t size);

        void writeFormId(const ESM::FormId&, bool wide = false, NAME tag = "FRMR");

    private:
        std::list<RecordData> mRecords;
        std::ostream* mStream;
        std::streampos mHeaderPos;
        ToUTF8::Utf8Encoder* mEncoder;
        int mRecordCount;
        bool mCounting;

        Header mHeader;

        void writeRefId(RefId value);
    };
}

#endif
