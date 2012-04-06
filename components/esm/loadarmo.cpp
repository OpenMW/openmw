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
        parts.push_back(pr);
    }
}

void PartReferenceList::save(ESMWriter &esm)
{
    for (std::vector<PartReference>::iterator it = parts.begin(); it != parts.end(); ++it)
    {
        esm.writeHT(it->part);
        esm.writeHNOString("BNAM", it->male);
        esm.writeHNOString("CNAM", it->female);
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

void Armor::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    esm.writeHNString("FNAM", name);
    esm.writeHNOString("SCRI", script);
    esm.writeHNT("AODT", data, 24);
    esm.writeHNOString("ITEX", icon);
    parts.save(esm);
    esm.writeHNOString("ENAM", enchant);
}

}
