#include "loadalch.hpp"

namespace ESM
{
void Potion::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    icon = esm.getHNOString("TEXT"); // not ITEX here for some reason
    script = esm.getHNOString("SCRI");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "ALDT", 12);
    effects.load(esm);
}
void Potion::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", model);
    esm.writeHNOCString("TEXT", icon);
    esm.writeHNOCString("SCRI", script);
    esm.writeHNOCString("FNAM", name);
    esm.writeHNT("ALDT", data, 12);
    effects.save(esm);
}
}
