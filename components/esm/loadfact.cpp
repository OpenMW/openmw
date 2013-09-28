#include "loadfact.hpp"

namespace ESM
{

void Faction::load(ESMReader &esm)
{
    name = esm.getHNString("FNAM");

    // Read rank names. These are optional.
    int i = 0;
    while (esm.isNextSub("RNAM") && i < 10)
        ranks[i++] = esm.getHString();

    // Main data struct
    esm.getHNT(data, "FADT", 240);

    if (data.isHidden > 1)
        esm.fail("Unknown flag!");

    // Read faction response values
    while (esm.hasMoreSubs())
    {
        Reaction r;
        r.faction = esm.getHNString("ANAM");
        esm.getHNT(r.reaction, "INTV");
        reactions.push_back(r);
    }
}

}
