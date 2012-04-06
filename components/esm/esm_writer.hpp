#ifndef _ESM_WRITER_H
#define _ESM_WRITER_H

#include <iostream>
#include <assert.h>

#include "esm_common.hpp"

namespace ESM {

class ESMWriter
{
public:
    void setVersion(Version ver);
    void setType(FileType type);

    void setAuthor(const std::string& author);
    void setDescription(const std::string& desc);

    void save(const std::string& file);
    void save(std::ostream& file);
    void close();

    void writeHNString(const std::string& name, const std::string& data);
    void writeHNOString(const std::string& name, const std::string& data)
    {
        if (!data.empty())
            writeHNString(name, data);
    }

    template<typename T>
    void writeHNT(const std::string& name, const T& data)
    {
        writeName(name);
        writeT(data);
    }

    template<typename T>
    void writeHNT(const std::string& name, const T& data, int size)
    {
        assert(sizeof(T) == size);
        writeHNT(name, data);
    }

    template<typename T>
    void writeHT(const T& data)
    {
        writeT((unsigned int)sizeof(T));
        writeT(data);
    }

    template<typename T>
    void writeHT(const T& data, int size)
    {
        assert(sizeof(T) == size);
        writeHT(data);
    }

    template<typename T>
    void writeT(const T& data)
    {
        write((char*)&data, sizeof(T));
    }

    template<typename T>
    void writeT(const T& data, int size)
    {
        assert(sizeof(T) == size);
        writeT(data);
    }

    void writeHString(const std::string& data);
    void writeName(const std::string& data);
    void write(const char* data, int size);

private:
    std::ostream m_stream;

    HEDRstruct m_header;
    SaveData m_saveData;
};

}
#endif
