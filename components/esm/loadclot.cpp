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
    esm.writeHNOString("FNAM", name);
    esm.writeHNT("CTDT", data, 12);

    esm.writeHNOString("SCRI", script);
    esm.writeHNOString("ITEX", icon);
    
    parts.save(esm);
    
    esm.writeHNOString("ENAM", enchant);
}

}
