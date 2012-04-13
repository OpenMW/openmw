#include "loadappa.hpp"

namespace ESM
{
void Apparatus::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "AADT", 16);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNString("ITEX");
}
void Apparatus::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", model);
    esm.writeHNCString("FNAM", name);
    esm.writeHNT("AADT", data, 16);
    esm.writeHNOCString("SCRI", script);
    esm.writeHNCString("ITEX", icon);
}
}
