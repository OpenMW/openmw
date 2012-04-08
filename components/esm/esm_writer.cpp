#include "esm_writer.hpp"
#include <fstream>
#include <cstring>

namespace ESM
{

void ESMWriter::setVersion(Version ver)
{
    m_header.version = ver;
}

void ESMWriter::setType(FileType type)
{
    m_header.type = type;
}

void ESMWriter::setAuthor(const std::string& auth)
{
    strncpy((char*)&m_header.author, auth.c_str(), 32);
}

void ESMWriter::setDescription(const std::string& desc)
{
    strncpy((char*)&m_header.desc, desc.c_str(), 256);
}

void ESMWriter::save(const std::string& file)
{
    std::ofstream fs(file.c_str(), std::ios_base::out | std::ios_base::trunc);
    save(fs);
    fs.close();
}

void ESMWriter::save(std::ostream& file)
{
    m_stream = &file;

    startRecord("TES3");
    writeT<int>(0);
    writeT<int>(0);

    endRecord();

    // TODO: Saving
}

void ESMWriter::close()
{
    // TODO: Saving
}

void ESMWriter::startRecord(const std::string& name)
{
    writeName(name);
    RecordData rec;
    rec.position = m_stream->tellp();
    rec.size = 0;
    m_records.push_back(rec);
    writeT<int>(0);
}

void ESMWriter::endRecord()
{
    std::streampos cur = m_stream->tellp();
    RecordData rec = m_records.back();
    m_records.pop_back();
    
    m_stream->seekp(rec.position);
    m_stream->write((char*)&rec.size, sizeof(int));

    m_stream->seekp(cur);
}

void ESMWriter::writeHNString(const std::string& name, const std::string& data)
{
    writeName(name);
    writeHString(data);
}

void ESMWriter::writeHString(const std::string& data)
{
    writeT<int>(data.size()-1);
    write(data.c_str(), data.size()-1);
}

void ESMWriter::writeName(const std::string& name)
{
    assert((name.size() == 4 && name[3] != '\0') || (name.size() == 5 && name[4] == '\0'));
    write(name.c_str(), name.size()-1);
}

void ESMWriter::write(const char* data, int size)
{
    if (!m_records.empty())
        m_records.back().size += size;

    m_stream->write(data, size);
}

}
