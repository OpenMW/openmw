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
    if (!icon.empty())
        esm.writeHNString("TEXT", icon);
    if (!script.empty())
        esm.writeHNString("SCRI", script);
    if (!name.empty())
        esm.writeHNString("FNAM", name);
    esm.writeHNT("ALDT", data, 12);
    effects.save(esm);
}
}
