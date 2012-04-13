#include "loaddoor.hpp"

namespace ESM
{

void Door::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    script = esm.getHNOString("SCRI");
    openSound = esm.getHNOString("SNAM");
    closeSound = esm.getHNOString("ANAM");
}
void Door::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", model);
    esm.writeHNOCString("FNAM", name);
    esm.writeHNOCString("SCRI", script);
    esm.writeHNOCString("SNAM", openSound);
    esm.writeHNOCString("ANAM", closeSound);
}

}
