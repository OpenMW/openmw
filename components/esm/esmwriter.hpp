#ifndef OPENMW_ESM_WRITER_H
#define OPENMW_ESM_WRITER_H

#include <iosfwd>
#include <list>

#include <components/to_utf8/to_utf8.hpp>

#include "esmcommon.hpp"
#include "loadtes3.hpp"

namespace ESM {

class ESMWriter
{
    struct RecordData
    {
        std::string name;
        std::streampos position;
        size_t size;
    };

public:
    int getVersion();
    void setVersion(int ver);
    void setEncoder(ToUTF8::Utf8Encoder *encoding); // Write strings as UTF-8?
    void setAuthor(const std::string& author);
    void setDescription(const std::string& desc);
    void setRecordCount (int count);
    void setFormat (int format);

    void addMaster(const std::string& name, uint64_t size);

    void save(const std::string& file);
    void save(std::ostream& file);
    void close();

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

    template<typename T>
    void writeT(const T& data, size_t size)
    {
        write((char*)&data, size);
    }

    void startRecord(const std::string& name, uint32_t flags);
    void startSubRecord(const std::string& name);
    void endRecord(const std::string& name);
    void writeHString(const std::string& data);
    void writeHCString(const std::string& data);
    void writeName(const std::string& data);
    void write(const char* data, size_t size);

private:
    std::list<RecordData> m_records;
    std::ostream* m_stream;
    std::streampos m_headerPos;
    ToUTF8::Utf8Encoder* m_encoder;
    int m_recordCount;

    Header mHeader;
};

}
#endif
