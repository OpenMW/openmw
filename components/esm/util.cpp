#include "util.hpp"

#include <cstdint>

namespace ESM
{
    bool readDeleSubRecord(ESMReader &esm)
    {
        if (esm.isNextSub("DELE"))
        {
            esm.skipHSub();
            return true;
        }
        return false;
    }

    void writeDeleSubRecord(ESMWriter &esm)
    {
        esm.writeHNT("DELE", static_cast<int32_t>(0));
    }
}
