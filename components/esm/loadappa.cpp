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
}
