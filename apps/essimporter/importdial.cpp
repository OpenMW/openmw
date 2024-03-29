#include "importdial.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void DIAL::load(ESM::ESMReader& esm)
    {
        // See ESM::Dialogue::Type enum, not sure why we would need this here though
        int32_t type = 0;
        esm.getHNOT(type, "DATA");

        // Deleted dialogue in a savefile. No clue what this means...
        int32_t deleted = 0;
        esm.getHNOT(deleted, "DELE");

        mIndex = 0;
        // *should* always occur except when the dialogue is deleted, but leaving it optional just in case...
        esm.getHNOT(mIndex, "XIDX");
    }

}
