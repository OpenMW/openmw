#include "file.hpp"

#include "stream.hpp"

namespace Bgsm
{
    void MaterialFile::read(BGSMStream& stream)
    {
    }

    void BGSMFile::read(BGSMStream& stream)
    {
        MaterialFile::read(stream);
    }

    void BGEMFile::read(BGSMStream& stream)
    {
        MaterialFile::read(stream);
    }
}
