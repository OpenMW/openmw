#include "loadligh.hpp"

namespace ESM
{

void Light::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    icon = esm.getHNOString("ITEX");
    assert(sizeof(data) == 24);
    esm.getHNT(data, "LHDT", 24);
    script = esm.getHNOString("SCRI");
    sound = esm.getHNOString("SNAM");
}
void Light::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    esm.writeHNOString("FNAM", name);
    esm.writeHNOString("ITEX", icon);
    esm.writeHNT("LHDT", data, 24);
    esm.writeHNOString("SCRI", script);
    esm.writeHNOString("SNAM", sound);
}

}
