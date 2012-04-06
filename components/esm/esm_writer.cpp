#include "esm_writer.hpp"

namespace ESM
{

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
    write(name.c_str(), name.size()-1);
}

void ESMWriter::write(const char* data, int size)
{
    m_stream.write(data, size);
}

}
