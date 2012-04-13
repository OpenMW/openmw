#include "loadmisc.hpp"

namespace ESM
{

void Miscellaneous::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "MCDT", 12);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
}
void Miscellaneous::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", model);
    esm.writeHNOCString("FNAM", name);
    esm.writeHNT("MCDT", data, 12);
    esm.writeHNOCString("SCRI", script);
    esm.writeHNOCString("ITEX", icon);
}

}
