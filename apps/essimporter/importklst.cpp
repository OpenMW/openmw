#include "importklst.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void KLST::load(ESM::ESMReader& esm)
    {
        while (esm.isNextSub("KNAM"))
        {
            std::string refId = esm.getHString();
            int32_t count;
            esm.getHNT(count, "CNAM");
            mKillCounter[refId] = count;
        }

        mWerewolfKills = 0;
        esm.getHNOT(mWerewolfKills, "INTV");
    }

}
