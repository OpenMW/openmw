#include "loadnpc.hpp"

namespace ESM
{

void NPC::load(ESMReader &esm, const std::string& id)
{
    mId = id;

    npdt52.gold = -10;

    model = esm.getHNOString("MODL");
    name = esm.getHNOString("FNAM");

    race = esm.getHNString("RNAM");
    cls = esm.getHNString("CNAM");
    faction = esm.getHNString("ANAM");
    head = esm.getHNString("BNAM");
    hair = esm.getHNString("KNAM");

    script = esm.getHNOString("SCRI");

    esm.getSubNameIs("NPDT");
    esm.getSubHeader();
    if (esm.getSubSize() == 52)
        esm.getExact(&npdt52, 52);
    else if (esm.getSubSize() == 12)
        esm.getExact(&npdt12, 12);
    else
        esm.fail("NPC_NPDT must be 12 or 52 bytes long");

    esm.getHNT(flags, "FLAG");

    inventory.load(esm);
    spells.load(esm);

    if (esm.isNextSub("AIDT"))
    {
        esm.getHExact(&AI, sizeof(AI));
        hasAI = true;
    }
    else
        hasAI = false;

    while (esm.isNextSub("DODT") || esm.isNextSub("DNAM")) {
        if (esm.retSubName() == 0x54444f44) { // DODT struct
            Dest dodt;
            esm.getHExact(&dodt.pos, 24);
            dest.push_back(dodt);
        } else if (esm.retSubName() == 0x4d414e44) { // DNAM struct
            dest.back().cellName = esm.getHString();
        } 
    }
    aiPack.load(esm);
    esm.skipRecord();
}

}
