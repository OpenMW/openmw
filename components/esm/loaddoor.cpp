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
    esm.writeHNString("MODL", model);
    esm.writeHNOString("FNAM", name);
    esm.writeHNOString("SCRI", script);
    esm.writeHNOString("SNAM", openSound);
    esm.writeHNOString("ANAM", closeSound);
}

}
