#include "loadclot.hpp"

namespace ESM
{

void Clothing::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "CTDT", 12);

    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");

    parts.load(esm);

    enchant = esm.getHNOString("ENAM");
}
void Clothing::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    if (!name.empty())
        esm.writeHNString("FNAM", name);
    esm.writeHNT("CTDT", data, 12);

    if (!script.empty())
        esm.writeHNString("SCRI", script);
    if (!icon.empty())
        esm.writeHNString("ITEX", icon);
    
    parts.save(esm);
    
    if (!enchant.empty())
        esm.writeHNString("ENAM", enchant);
}

}
