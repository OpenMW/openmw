#include "stream.hpp"

namespace Bgsm
{
    void BGSMStream::read(std::string& str)
    {
        // Strings this long are not expected in BGSM
        str = readSizedString(1024);
        std::size_t end = str.find('\0');
        if (end != std::string::npos)
            str.erase(end);
    }
}
