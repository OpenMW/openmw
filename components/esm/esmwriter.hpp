#ifndef OPENMW_ESM_WRITER_H
#define OPENMW_ESM_WRITER_H

#include <iosfwd>
#include <list>

#include "esmcommon.hpp"
#include "loadtes3.hpp"

namespace ToUTF8
{
    class Utf8Encoder;
}

namespace ESM {

class ESMWriter
{
        struct RecordData
        {
            std::string name;
            std::streampos position;
            uint32_t size;
        };

    public:

        ESMWriter();

        unsigned int getVersion() const;

        // Set various header data (ESM::Header::Data). All of the below functions must be called before writing,
        // otherwise this data will be left uninitialized.
        void setVersion(unsigned int ver = 0x3fa66666);
        void setType(int type);
        void setEncoder(ToUTF8::Utf8Encoder *encoding);
        void setAuthor(const std::string& author);
        void setDescription(const std::string& desc);

        // Set the record count for writing it in the file header
        void setRecordCount (int count);
        // Counts how many records we have actually written.
        // It is a good idea to compare this with the value you wrote into the header (setRecordCount)
        // It should be the record count you set + 1 (1 additional record for the TES3 header)
        int getRecordCount() { return mRecordCount; }
        void setFormat (int format);

        void clearMaster();

        void addMaster(const std::string& name, uint64_t size);

        void save(std::ostream& file);
        ///< Start saving a file by writing the TES3 header.

        void close();
        ///< \note Does not close the stream.

        void writeHNString(const std::string& name, const std::string& data);
        void writeHNString(const std::string& name, const std::string& data, size_t size);
        void writeHNCString(const std::string& name, const std::string& data)
        {
            startSubRecord(name);
            writeHCString(data);
            endRecord(name);
        }
        void writeHNOString(const std::string& name, const std::string& data)
        {
            if (!data.empty())
                writeHNString(name, data);
        }
        void writeHNOCString(const std::string& name, const std::string& data)
        {
            if (!data.empty())
                writeHNCString(name, data);
        }

        template<typename T>
        void writeHNT(const std::string& name, const T& data)
        {
            startSubRecord(name);
            writeT(data);
            endRecord(name);
        }

        template<typename T, std::size_t size>
        void writeHNT(const std::string& name, const T (&data)[size])
        {
            startSubRecord(name);
            writeT(data);
            endRecord(name);
        }

        // Prevent using writeHNT with strings. This already happened by accident and results in
        // state being discarded without any error on writing or reading it. :(
        // writeHNString and friends must be used instead.
        void writeHNT(const std::string& name, const std::string& data) = delete;

        void writeT(const std::string& data) = delete;

        template<typename T, std::size_t size>
        void writeHNT(const std::string& name, const T (&data)[size], int) = delete;

        template<typename T>
        void writeHNT(const std::string& name, const T& data, int size)
        {
            startSubRecord(name);
            writeT(data, size);
            endRecord(name);
        }

        template<typename T>
        void writeT(const T& data)
        {
            write((char*)&data, sizeof(T));
        }

        template<typename T, std::size_t size>
        void writeT(const T (&data)[size])
        {
            write(reinterpret_cast<const char*>(data), size * sizeof(T));
        }

        template<typename T>
        void writeT(const T& data, size_t size)
        {
            write((char*)&data, size);
        }

        void startRecord(const std::string& name, uint32_t flags = 0);
        void startRecord(uint32_t name, uint32_t flags = 0);
        /// @note Sub-record hierarchies are not properly supported in ESMReader. This should be fixed later.
        void startSubRecord(const std::string& name);
        void endRecord(const std::string& name);
        void endRecord(uint32_t name);
        void writeFixedSizeString(const std::string& data, int size);
        void writeHString(const std::string& data);
        void writeHCString(const std::string& data);
        void writeName(const std::string& data);
        void write(const char* data, size_t size);

    private:
        std::list<RecordData> mRecords;
        std::ostream* mStream;
        std::streampos mHeaderPos;
        ToUTF8::Utf8Encoder* mEncoder;
        int mRecordCount;
        bool mCounting;

        Header mHeader;
    };
}

#endif
