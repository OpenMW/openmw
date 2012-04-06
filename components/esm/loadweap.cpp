#include "loadweap.hpp"

namespace ESM
{

void Weapon::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "WPDT", 32);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
    enchant = esm.getHNOString("ENAM");
}
void Weapon::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    esm.writeHNOString("FNAM", name);
    esm.writeHNT("WPDT", data, 32);
    esm.writeHNOString("SCRI", script);
    esm.writeHNOString("ITEX", icon);
    esm.writeHNOString("ENAM", enchant);
}

}
