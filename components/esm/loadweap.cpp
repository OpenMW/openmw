#include "loadweap.hpp"

namespace ESM
{

void Weapon::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "WPDT", 32);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
    enchant = esm.getHNOString("ENAM");
}

}
