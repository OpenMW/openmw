#include "loadarmo.hpp"

namespace ESM
{

void PartReferenceList::load(ESMReader &esm)
{
    while (esm.isNextSub("INDX"))
    {
        PartReference pr;
        esm.getHT(pr.part); // The INDX byte
        pr.male = esm.getHNOString("BNAM");
        pr.female = esm.getHNOString("CNAM");
    }
}

void Armor::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    script = esm.getHNOString("SCRI");
    esm.getHNT(data, "AODT", 24);
    icon = esm.getHNOString("ITEX");
    parts.load(esm);
    enchant = esm.getHNOString("ENAM");
}

}
