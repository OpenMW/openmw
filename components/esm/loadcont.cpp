#include "loadcont.hpp"

namespace ESM
{

void InventoryList::load(ESMReader &esm)
{
    ContItem ci;
    while (esm.isNextSub("NPCO"))
    {
        esm.getHT(ci, 36);
        list.push_back(ci);
    }
}

void InventoryList::save(ESMWriter &esm)
{
    for (std::vector<ContItem>::iterator it = list.begin(); it != list.end(); ++it)
    {
        esm.writeHNT("NPCO", *it, 36);
    }
}

void Container::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(weight, "CNDT", 4);
    esm.getHNT(flags, "FLAG", 4);

    if (flags & 0xf4)
        esm.fail("Unknown flags");
    if (!(flags & 0x8))
        esm.fail("Flag 8 not set");

    script = esm.getHNOString("SCRI");

    inventory.load(esm);
}

void Container::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    if (!name.empty())
        esm.writeHNString("FNAM", name);
    esm.writeHNT("CNDT", weight, 4);
    esm.writeHNT("FLAG", flags, 4);

    if (!script.empty())
        esm.writeHNString("SCRI", script);

    inventory.save(esm);
}

}
