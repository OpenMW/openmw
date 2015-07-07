#include "util.hpp"

namespace ESM
{
    bool readDeleSubRecord(ESMReader &esm)
    {
        if (esm.isNextSub("DELE"))
        {
            esm.getSubName();
            esm.skipHSub();
            return true;
        }
        return false;
    }

    void writeDeleSubRecord(ESMWriter &esm)
    {
        esm.writeHNString("DELE", "");
    }
}
