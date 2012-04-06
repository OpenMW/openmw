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
        type = Type_Repair;
        // Swap t.data.quality and t.data.uses for repair items (sigh)
        float tmp = *((float*) &data.uses);
        data.uses = *((int*) &data.quality);
        data.quality = tmp;
    }
    else if (n == "LKDT")
        type = Type_Pick;
    else if (n == "PBDT")
        type = Type_Probe;

    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
}
void Tool::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    esm.writeHNString("FNAM", name);
    
    switch(type)
    {
    case Type_Repair: esm.writeHString("RIDT"); break;
    case Type_Pick: esm.writeHString("LKDT"); break;
    case Type_Probe: esm.writeHString("PBDT"); break;
    }

    esm.writeT(data, 16);
    esm.writeHNOString("SCRI", script);
    esm.writeHNOString("ITEX", icon);
}


}
