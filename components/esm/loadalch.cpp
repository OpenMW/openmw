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
    esm.writeHNString("MODL", model);
    esm.writeHNOString("TEXT", icon);
    esm.writeHNOString("SCRI", script);
    esm.writeHNOString("FNAM", name);
    esm.writeHNT("ALDT", data, 12);
    effects.save(esm);
}
}
