#include "loadalch.hpp"

namespace ESM
{
void Potion::load(ESMReader &esm, const std::string& id)
{
    mId = id;

    model = esm.getHNString("MODL");
    icon = esm.getHNOString("TEXT"); // not ITEX here for some reason
    script = esm.getHNOString("SCRI");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "ALDT", 12);
    effects.load(esm);
}
}
