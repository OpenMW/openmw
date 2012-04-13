#include "loadingr.hpp"

namespace ESM
{

void Ingredient::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "IRDT", 56);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
}
void Ingredient::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", model);
    esm.writeHNCString("FNAM", name);
    esm.writeHNT("IRDT", data, 56);
    esm.writeHNOCString("SCRI", script);
    esm.writeHNOCString("ITEX", icon);
}

}
