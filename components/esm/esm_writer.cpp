#include "esm_writer.hpp"

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
    strcpy(auth.c_str(), m_header.author, 32);
}

void ESMWriter::setDescription(const std::string& desc)
{
    strcpy(desc.c_str(), m_header.desc, 256);
}

void ESMWriter::save(const std::string& file)
{
    std::ostream os(file, "wb");
    save(os);
}

void ESMWriter::save(std::ostream& file)
{
    // TODO: Saving
}

void ESMWriter::close()
{
    // TODO: Saving
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
    m_stream.write(data, size);
}

}
