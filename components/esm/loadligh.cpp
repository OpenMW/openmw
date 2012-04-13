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
    esm.writeHNCString("MODL", model);
    esm.writeHNOCString("FNAM", name);
    esm.writeHNOCString("ITEX", icon);
    esm.writeHNT("LHDT", data, 24);
    esm.writeHNOCString("SCRI", script);
    esm.writeHNOCString("SNAM", sound);
}

}
