#include "esmwriter.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

bool count = true;

namespace ESM
{

int ESMWriter::getVersion()
{
    return mHeader.mData.version;
}

void ESMWriter::setVersion(int ver)
{
    mHeader.mData.version = ver;
}

void ESMWriter::setAuthor(const std::string& auth)
{
    mHeader.mData.author.assign (auth);
}

void ESMWriter::setDescription(const std::string& desc)
{
    mHeader.mData.desc.assign (desc);
}

void ESMWriter::setRecordCount (int count)
{
    mHeader.mData.records = count;
}

void ESMWriter::setFormat (int format)
{
    mHeader.mFormat = format;
}

void ESMWriter::addMaster(const std::string& name, uint64_t size)
{
    Header::MasterData d;
    d.name = name;
    d.size = size;
    mHeader.mMaster.push_back(d);
}

void ESMWriter::save(const std::string& file)
{
    std::ofstream fs(file.c_str(), std::ios_base::out | std::ios_base::trunc);
    save(fs);
}

void ESMWriter::save(std::ostream& file)
{
    m_recordCount = 0;
    m_stream = &file;

    startRecord("TES3", 0);

    mHeader.save (*this);

    endRecord("TES3");
}

void ESMWriter::close()
{
    m_stream->flush();

    if (!m_records.empty())
        throw "Unclosed record remaining";
}

void ESMWriter::startRecord(const std::string& name, uint32_t flags)
{
    m_recordCount++;

    writeName(name);
    RecordData rec;
    rec.name = name;
    rec.position = m_stream->tellp();
    rec.size = 0;
    writeT<int>(0); // Size goes here
    writeT<int>(0); // Unused header?
    writeT(flags);
    m_records.push_back(rec);

    assert(m_records.back().size == 0);
}

void ESMWriter::startSubRecord(const std::string& name)
{
    writeName(name);
    RecordData rec;
    rec.name = name;
    rec.position = m_stream->tellp();
    rec.size = 0;
    writeT<int>(0); // Size goes here
    m_records.push_back(rec);

    assert(m_records.back().size == 0);
}

void ESMWriter::endRecord(const std::string& name)
{
    RecordData rec = m_records.back();
    assert(rec.name == name);
    m_records.pop_back();

    m_stream->seekp(rec.position);

    count = false;
    write((char*)&rec.size, sizeof(int));
    count = true;

    m_stream->seekp(0, std::ios::end);

}

void ESMWriter::writeHNString(const std::string& name, const std::string& data)
{
    startSubRecord(name);
    writeHString(data);
    endRecord(name);
}

void ESMWriter::writeHNString(const std::string& name, const std::string& data, size_t size)
{
    assert(data.size() <= size);
    startSubRecord(name);
    writeHString(data);

    if (data.size() < size)
    {
        for (size_t i = data.size(); i < size; ++i)
            write("\0",1);
    }

    endRecord(name);
}

void ESMWriter::writeHString(const std::string& data)
{
    if (data.size() == 0)
        write("\0", 1);
    else
    {
        // Convert to UTF8 and return
        std::string ascii = m_encoder->getLegacyEnc(data);

        write(ascii.c_str(), ascii.size());
    }
}

void ESMWriter::writeHCString(const std::string& data)
{
    writeHString(data);
    if (data.size() > 0 && data[data.size()-1] != '\0')
        write("\0", 1);
}

void ESMWriter::writeName(const std::string& name)
{
    assert((name.size() == 4 && name[3] != '\0'));
    write(name.c_str(), name.size());
}

void ESMWriter::write(const char* data, size_t size)
{
    if (count && !m_records.empty())
    {
        for (std::list<RecordData>::iterator it = m_records.begin(); it != m_records.end(); ++it)
            it->size += size;
    }

    m_stream->write(data, size);
}

void ESMWriter::setEncoder(ToUTF8::Utf8Encoder* encoder)
{
    m_encoder = encoder;
}

}
