#include "loadmisc.hpp"

namespace ESM
{

void Misc::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "MCDT", 12);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
}

}
