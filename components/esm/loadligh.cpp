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
    if (!name.empty())
        esm.writeHNString("FNAM", name);
    if (!icon.empty())
        esm.writeHNString("ITEX", icon);
    esm.writeHNT("LHDT", data, 24);
    if (!script.empty())
        esm.writeHNString("SCRI", script);
    if (!sound.empty())
        esm.writeHNString("SNAM", sound);
}

}
