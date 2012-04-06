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
    if (!name.empty())
        esm.writeHNString("FNAM", name);
    if (!script.empty())
        esm.writeHNString("SCRI", script);
    if (!openSound.empty())
        esm.writeHNString("SNAM", openSound);
    if (!closeSound.empty())
        esm.writeHNString("ANAM", closeSound);
}

}
