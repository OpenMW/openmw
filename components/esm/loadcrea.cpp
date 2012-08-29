#include "loadcrea.hpp"

namespace ESM {

void Creature::load(ESMReader &esm, const std::string& id)
{
    mId = id;

    model = esm.getHNString("MODL");
    original = esm.getHNOString("CNAM");
    name = esm.getHNOString("FNAM");
    script = esm.getHNOString("SCRI");

    esm.getHNT(data, "NPDT", 96);

    esm.getHNT(flags, "FLAG");
    scale = 1.0;
    esm.getHNOT(scale, "XSCL");

    inventory.load(esm);
    spells.load(esm);

    if (esm.isNextSub("AIDT"))
    {
        esm.getHExact(&AI, sizeof(AI));
        hasAI = true;
    }
    else
        hasAI = false;

    aiPack.load(esm);
    esm.skipRecord();
}

}
