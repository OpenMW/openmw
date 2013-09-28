#include "loadlocks.hpp"

namespace ESM
{

void Tool::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");

    esm.getSubName();
    NAME n = esm.retSubName();
    // The data name varies, RIDT for repair items, LKDT for lock
    // picks, PBDT for probes

    esm.getHT(data, 16);

    if (n == "RIDT")
    {
        // Swap t.data.quality and t.data.uses for repair items (sigh)
        float tmp = *((float*) &data.uses);
        data.uses = *((int*) &data.quality);
        data.quality = tmp;
    }

    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
}

}
