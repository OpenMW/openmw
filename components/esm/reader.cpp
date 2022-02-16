#include "reader.hpp"

//#ifdef NDEBUG
//#undef NDEBUG
//#endif

#include <cassert>
#include <stdexcept>

#include <components/files/constrainedfilestream.hpp>

#include "components/esm3/esmreader.hpp"
#include "components/esm4/reader.hpp"

namespace ESM
{
    Reader* Reader::getReader(const std::string &filename)
    {
        Files::IStreamPtr esmStream(Files::openConstrainedFileStream (filename.c_str ()));

        std::uint32_t modVer = 0; // get the first 4 bytes of the record header only
        esmStream->read((char*)&modVer, sizeof(modVer));
        if (esmStream->gcount() == sizeof(modVer))
        {
            esmStream->seekg(0);

            if (modVer == ESM4::REC_TES4)
            {
                return new ESM4::Reader(esmStream, filename);
            }
            else
            {
                //return new ESM3::ESMReader(esmStream, filename);
            }
        }

        throw std::runtime_error("Unknown file format");
    }

    bool Reader::getStringImpl(std::string& str, std::size_t size,
            Files::IStreamPtr filestream, ToUTF8::StatelessUtf8Encoder* encoder, bool hasNull)
    {
        std::size_t newSize = size;

        if (encoder)
        {
            std::string input(size, '\0');
            filestream->read(input.data(), size);
            if (filestream->gcount() == static_cast<std::streamsize>(size))
            {
                encoder->getUtf8(input, ToUTF8::BufferAllocationPolicy::FitToRequiredSize, str);
                return true;
            }
        }
        else
        {
            if (hasNull)
                newSize -= 1; // don't read the null terminator yet

            str.resize(newSize); // assumed C++11
            filestream->read(&str[0], newSize);
            if ((std::size_t)filestream->gcount() == newSize)
            {
                if (hasNull)
                {
                    char ch;
                    filestream->read(&ch, 1); // read the null terminator
                    assert (ch == '\0'
                            && "ESM4::Reader::getString string is not terminated with a null");
                }
#if 0
                else
                {
                    // NOTE: normal ESMs don't but omwsave has locals or spells with null terminator
                    assert (str[newSize - 1] != '\0'
                            && "ESM4::Reader::getString string is unexpectedly terminated with a null");
                }
#endif
                return true;
            }
        }

        str.clear();
        return false; // FIXME: throw instead?
    }
}
