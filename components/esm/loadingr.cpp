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
    esm.writeHNString("MODL", model);
    esm.writeHNString("FNAM", name);
    esm.writeHNT("IRDT", data, 56);
    if (!script.empty())
        esm.writeHNString("SCRI", script);
    if (!icon.empty())
        esm.writeHNString("ITEX", script);
}

}
